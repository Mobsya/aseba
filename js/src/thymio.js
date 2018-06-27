/* Wrap a promise to allow external resolve */

import {flatbuffers} from './flatbuffers';
import {mobsya} from './thymio_generated';

export class Request extends Promise {
    constructor(promise) {
        super(promise)
    }

    get node_id() {
        return this._node_id
    }

    _trigger_error(err) {
        this._onerror(err)
    }

    _trigger_then(...args) {
        return this._then.apply(this, args);
    }

    static create(request_id, node_id) {
        var then    = undefined;
        var onerror = undefined;
        let p = new Request(function(resolve, reject){
            then    = resolve;
            onerror = reject;
        }.bind(this));
        p._then = then
        p._onerror = onerror
        p._request_id = request_id
        p._node_id = node_id
        return p
    }
}

console.log(mobsya)
console.log(flatbuffers)

Request.ErrorType = mobsya.fb.ErrorType;

class AsebaVMDescription {
    constructor() {
        this.bytecode_size = this.data_size = this.stack_size = 0;
        this.variables = []
        this.functions = []
        this.events    = []
    }
}


export class Node {
    constructor(client, id, status) {
        this._id = id;
        this._status = status;
        this._desc   = undefined;
        this._client = client
    }
    get id() {
        return this._id
    }

    get status() {
        return this._status
    }

    get disconnected() {
        return this._status == mobsya.fb.NodeStatus.disconnected
    }

    get status_str() {
        switch(this.status) {
            case mobsya.fb.NodeStatus.connected: return "connected"
            case mobsya.fb.NodeStatus.ready: return "ready"
            case mobsya.fb.NodeStatus.busy: return "busy"
            case mobsya.fb.NodeStatus.disconnected: return "disconnected"
        }
        return "unknow"
    }

    async lock() {
        return await this._client.lock_node(this._id)
    }

    async unlock() {
        return await this._client.unlock_node(this._id)
    }

    async get_description() {
        if(!this._desc)
            this._desc = await this._client.request_aseba_vm_description(this._id);
        return this._desc
    }

    async send_aseba_program(code) {
        return await this._client.send_aseba_program(this._id, code);
    }

    async run_aseba_program() {
        return await this._client.run_aseba_program(this._id);
    }

    _set_status(status) {
        if(status != this._status) {
            this._status = status
            if(this.on_status_changed) {
                this.on_status_changed(status)
            }
        }
    }
}
Node.Status = mobsya.fb.NodeStatus;

export class Client {

    constructor(url) {
        this._socket = new WebSocket(url)
        this._socket.binaryType = 'arraybuffer';
        this._socket.onopen = this.onopen.bind(this)
        this._socket.onmessage = this.onmessage.bind(this)

        //In progress requests (id : node)
        this._requests = new Map();
        //Known nodes (id : node)
        this._nodes    = new Map();
    }

    onopen(event) {
        console.log("connected")
    }

    onmessage (event) {
        let data = new Uint8Array(event.data)
        let buf  = new flatbuffers.ByteBuffer(data);

        let message = mobsya.fb.Message.getRootAsMessage(buf, null)
        switch(message.messageType()) {
            case mobsya.fb.AnyMessage.NodesChanged: {
                this.on_nodes_changed(this._nodes_changed_as_node_list(message.message(new mobsya.fb.NodesChanged())))
                break;
            }
            case mobsya.fb.AnyMessage.NodeAsebaVMDescription: {
                let msg = message.message(new mobsya.fb.NodeAsebaVMDescription())
                let req = this._get_request(msg.requestId())
                const id = msg.nodeId()
                if(req) {
                    req._trigger_then(id, this._unserialize_aseba_vm_description(msg))
                }
                break;
            }
            case mobsya.fb.AnyMessage.RequestCompleted: {
                let msg = message.message(new mobsya.fb.RequestCompleted())
                let req = this._get_request(msg.requestId())
                if(req) {
                    req._trigger_then()
                }
                break;
            }
            case mobsya.fb.AnyMessage.Error: {
                let msg = message.message(new mobsya.fb.Error())
                let req = this._get_request(msg.requestId())
                if(req) {
                    req._trigger_error(msg.error())
                }
                break
            }
            case mobsya.fb.AnyMessage.CompilationError: {
                let msg = message.message(new mobsya.fb.CompilationError())
                let req = this._get_request(msg.requestId())
                if(req) {
                    req._trigger_error(null)
                }
                break
            }
        }
    }

    /* request the description of the aseba vm for the node with the given id */
    request_aseba_vm_description(id) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()

        mobsya.fb.RequestNodeAsebaVMDescription.startRequestNodeAsebaVMDescription(builder)
        mobsya.fb.RequestNodeAsebaVMDescription.addRequestId(builder, req_id)
        mobsya.fb.RequestNodeAsebaVMDescription.addNodeId(builder, id)
        const offset = mobsya.fb.RequestNodeAsebaVMDescription.endRequestNodeAsebaVMDescription(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.RequestNodeAsebaVMDescription)
        return this._prepare_request(req_id, id)
    }

    send_aseba_program(id, code) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()
        const codeOffset = builder.createString(code)
        mobsya.fb.RequestAsebaCodeLoad.startRequestAsebaCodeLoad(builder)
        mobsya.fb.RequestAsebaCodeLoad.addRequestId(builder, req_id)
        mobsya.fb.RequestAsebaCodeLoad.addNodeId(builder, id)
        mobsya.fb.RequestAsebaCodeLoad.addProgram(builder, codeOffset)
        const offset = mobsya.fb.RequestAsebaCodeLoad.endRequestAsebaCodeLoad(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.RequestAsebaCodeLoad)
        return this._prepare_request(req_id, id)
    }

    run_aseba_program(id) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()
        mobsya.fb.RequestAsebaCodeRun.startRequestAsebaCodeRun(builder)
        mobsya.fb.RequestAsebaCodeRun.addRequestId(builder, req_id)
        mobsya.fb.RequestAsebaCodeRun.addNodeId(builder, id)
        const offset = mobsya.fb.RequestAsebaCodeRun.endRequestAsebaCodeRun(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.RequestAsebaCodeRun)
        return this._prepare_request(req_id, id)
    }

    /* request the description of the aseba vm for the node with the given id */
    lock_node(id) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()

        mobsya.fb.LockNode.startLockNode(builder)
        mobsya.fb.LockNode.addRequestId(builder, req_id)
        mobsya.fb.LockNode.addNodeId(builder, id)
        let offset = mobsya.fb.LockNode.endLockNode(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.LockNode)
        return this._prepare_request(req_id, id)
    }

    unlock_node(id) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()

        mobsya.fb.UnlockNode.startUnlockNode(builder)
        mobsya.fb.UnlockNode.addRequestId(builder, req_id)
        mobsya.fb.UnlockNode.addNodeId(builder, id)
        let offset = mobsya.fb.UnlockNode.endUnlockNode(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.UnlockNode)
        return this._prepare_request(req_id, id)
    }

    _nodes_changed_as_node_list(msg) {
        let nodes = []
        for(let i = 0; i < msg.nodesLength(); i++) {
            const n = msg.nodes(i);
            let node = this._requests.get(n.nodeId())
            if(!node) {
                node = new Node(this, n.nodeId(), n.status())
                this._requests.set(n.nodeId(), node)
            }
            if(n.status() == Node.Status.disconnected) {
                this._requests.delete(n.nodeId())
            }
            nodes.push(node)
            node._set_status(n.status())
        }
        return nodes
    }

    _unserialize_aseba_vm_description(msg) {
        let desc = new AsebaVMDescription()
        desc.bytecode_size = msg.bytecodeSize()
        desc.data_size  = msg.dataSize()
        desc.stack_size = msg.stackSize()

        for(let i = 0; i < msg.variablesLength(); i++) {
            const v = msg.variables(i);
            desc.variables.push({"name" : v.name(), "size" : v.size()})
        }

        for(let i = 0; i < msg.eventsLength(); i++) {
            const v = msg.events(i);
            desc.events.push({"name" : v.name(), "description" : v.description()})
        }

        for(let i = 0; i < msg.functionsLength(); i++) {
            const v = msg.functions(i);
            let params = []
            for(let j = 0; j < v.parametersLength(); j++) {
                const p = v.parameters(i);
                params.push({"name" : v.name(), "size" : p.size()})
            }
            desc.functions.push({"name" : v.name(), "description" : v.description(), "params": params})
        }
        return desc
    }

    _wrap_message_and_send(builder, offset, type) {
        mobsya.fb.Message.startMessage(builder)
        mobsya.fb.Message.addMessageType(builder, type)
        mobsya.fb.Message.addMessage(builder, offset)
        const builtMsg = mobsya.fb.Message.endMessage(builder)
        builder.finish(builtMsg);
        this._socket.send(builder.asUint8Array())
    }

    _gen_request_id(min, max) {
        let n = 0
        do {
            n = Math.floor(Math.random() * (0xffffffff - 2)) + 1
        }while(this._requests.has(n))
        return n
    }

    _get_request(id) {
        const req = this._requests.get(id)
        if(req != undefined)
            this._requests.delete(id)
            console.log(id, req)
            if(req == undefined) {
                console.error(`unknown request ${id}`)
            }
            return req
    }
    _prepare_request(req_id, node_id) {
        let req = Request.create(req_id, node_id)
        this._requests.set(req_id, req)
        return req
    }
}
