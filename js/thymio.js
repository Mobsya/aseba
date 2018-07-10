/* Wrap a promise to allow external resolve */

/** @module Mobsya/tymio */

import {flatbuffers} from 'flatbuffers';
import {mobsya} from './thymio_generated';
import WebSocket from 'isomorphic-ws';

/** Class representing Request.
 *  A Request wraps a promise that will be triggered when the corresponding Error/RequestCompleted message get received
 *  @private
 */


/**
 * The built in string object.
 * @external String
 * @see {@link https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String|String}
 */
/**
 * The built in Promise object.
 * @external Promise
 * @see {@link https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise|Promise}
 */
export class Request {

    constructor(request_id) {
        var then    = undefined;
        var onerror = undefined;
        this._promise = new Promise((resolve, reject) => {
            then    = resolve;
            onerror = reject;
        });
        this._then = then
        this._onerror = onerror
        this._request_id = request_id
    }

    _trigger_error(err) {
        this._onerror(err)
    }

    _trigger_then(...args) {
        return this._then.apply(this, args);
    }
}

/** Error type of a failed request */
Request.ErrorType = mobsya.fb.ErrorType;


/** Description of an aseba virtual machine. */
class AsebaVMDescription {
    constructor() {
        this.bytecode_size = this.data_size = this.stack_size = 0;
        this.variables = []
        this.functions = []
        this.events    = []
    }
}


/** Node */
export class Node {
    constructor(client, id, status) {
        this._id = id;
        this._status = status;
        this._desc   = undefined;
        this._client = client
    }

    /** return the node id*/
    get id() {
        return this._id
    }

    /** The node status
     *  @type {mobsya.fb.NodeStatus}
     */
    get status() {
        return this._status
    }

    /** Whether the node is ready (connected, and locked)
     *  @type {boolean}
     */
    get ready() {
        return this._status == mobsya.fb.NodeStatus.ready
    }

    /** The node status converted to string.
     *  @type {string}
     */
    get status_str() {
        switch(this.status) {
            case mobsya.fb.NodeStatus.connected: return "connected"
            case mobsya.fb.NodeStatus.ready: return "ready"
            case mobsya.fb.NodeStatus.available: return "available"
            case mobsya.fb.NodeStatus.busy: return "busy"
            case mobsya.fb.NodeStatus.disconnected: return "disconnected"
        }
        return "unknow"
    }

    /** Lock the device
     *  Locking a device is akin to take sole ownership of it until the connection
     *  is closed or the unlock method is explicitely called
     *
     *  The device must be in the available state before it can be locked.
     *  Once a device is locked, all client will see the device becoming busy.
     *
     *  If the device can not be locked, an {@link mobsya.fb.Error} is raised.
     *  @throws {mobsya.fb.Error}
     */
    async lock() {
        await this._client.lock_node(this._id)
        this._status = Node.Status.ready
    }

    /** Unlock the device
     *  Once a device is unlocked, all client will see the device becoming available.
     *  Once unlock, a device can't be written to until loc
     *  @throws {mobsya.fb.Error}
     *  @see lock
     */
    async unlock() {
        return await this._client.unlock_node(this._id)
        this._status = Node.Status.available
    }

    /** Get the description from the device
     *  The device must be in the available state before requesting the VM.
     *  @returns {external:Promise<AsebaVMDescription>}
     *  @throws {mobsya.fb.Error}
     *  @see lock
     */
    async get_description() {
        if(!this._desc)
            this._desc = await this._client.request_aseba_vm_description(this._id);
        return this._desc
    }

    /** Load an aseba program on the VM
     *  The device must be locked & ready before calling this function
     *  @param {external:String} code - the aseba code to load
     *  @throws {mobsya.fb.Error}
     *  @see lock
     */
    async send_aseba_program(code) {
        return await this._client.send_aseba_program(this._id, code);
    }

    /** Run the code currently loaded on the vm
     *  The device must be locked & ready before calling this function
     *  @throws {mobsya.fb.Error}
     *  @see lock
     */
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

class InvalidNodeIDException {
    constructor() {
        this.message = message;
        this.name = 'InvalidNodeIDException';
    }
}

class NodeId {
    constructor(array) {
        if(array.length != 16) {
            throw new InvalidNodeIDException()
        }
        this.data = array
    }
    toString() {
        let res = []
        this.data.forEach(byte => {
                res.push(byte.toString(16).padStart(2, '0'))
                if([4, 7, 10, 13].includes(res.length))
                    res.push('-');
            }
        );
        return '{' + res.join('') + '}';
    }
}

/*
 * The status of a node
 */
Node.Status = mobsya.fb.NodeStatus;

/*
 * A client. Main entry point of the api
 */
export class Client {

    /**
     *  @param {external:String} url : Web socket address
     *  @see lock
     */
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

    /**
     *  @param {Node[]} nodes : Nodes whose status has changed.
     *
     */
    on_nodes_changed(nodes) {

    }

    onopen() {
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
                const id = this._id(msg.nodeId())
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

    request_aseba_vm_description(id) {
        const builder = new flatbuffers.Builder();
        const req_id  = this._gen_request_id()
        const nodeOffset = this._create_node_id(builder, id)

        mobsya.fb.RequestNodeAsebaVMDescription.startRequestNodeAsebaVMDescription(builder)
        mobsya.fb.RequestNodeAsebaVMDescription.addRequestId(builder, req_id)
        mobsya.fb.RequestNodeAsebaVMDescription.addNodeId(builder, nodeOffset)
        const offset = mobsya.fb.RequestNodeAsebaVMDescription.endRequestNodeAsebaVMDescription(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.RequestNodeAsebaVMDescription)
        return this._prepare_request(req_id)
    }

    send_aseba_program(id, code) {
        const builder = new flatbuffers.Builder();
        const req_id  = this._gen_request_id()
        const codeOffset = builder.createString(code)
        const nodeOffset = this._create_node_id(builder, id)
        mobsya.fb.RequestAsebaCodeLoad.startRequestAsebaCodeLoad(builder)
        mobsya.fb.RequestAsebaCodeLoad.addRequestId(builder, req_id)
        mobsya.fb.RequestAsebaCodeLoad.addNodeId(builder, nodeOffset)
        mobsya.fb.RequestAsebaCodeLoad.addProgram(builder, codeOffset)
        const offset = mobsya.fb.RequestAsebaCodeLoad.endRequestAsebaCodeLoad(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.RequestAsebaCodeLoad)
        return this._prepare_request(req_id)
    }

    run_aseba_program(id) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()
        const nodeOffset = this._create_node_id(builder, id)
        mobsya.fb.RequestAsebaCodeRun.startRequestAsebaCodeRun(builder)
        mobsya.fb.RequestAsebaCodeRun.addRequestId(builder, req_id)
        mobsya.fb.RequestAsebaCodeRun.addNodeId(builder, nodeOffset)
        const offset = mobsya.fb.RequestAsebaCodeRun.endRequestAsebaCodeRun(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.RequestAsebaCodeRun)
        return this._prepare_request(req_id)
    }

    /* request the description of the aseba vm for the node with the given id */
    lock_node(id) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()
        const nodeOffset = this._create_node_id(builder, id)

        mobsya.fb.LockNode.startLockNode(builder)
        mobsya.fb.LockNode.addRequestId(builder, req_id)
        mobsya.fb.LockNode.addNodeId(builder, nodeOffset)
        let offset = mobsya.fb.LockNode.endLockNode(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.LockNode)
        return this._prepare_request(req_id)
    }

    unlock_node(id) {
        let builder = new flatbuffers.Builder();
        let req_id  = this._gen_request_id()
        const nodeOffset = this._create_node_id(builder, id)

        mobsya.fb.UnlockNode.startUnlockNode(builder)
        mobsya.fb.UnlockNode.addRequestId(builder, req_id)
        mobsya.fb.UnlockNode.addNodeId(builder, nodeOffset)
        let offset = mobsya.fb.UnlockNode.endUnlockNode(builder)
        this._wrap_message_and_send(builder, offset, mobsya.fb.AnyMessage.UnlockNode)
        return this._prepare_request(req_id)
    }

    _nodes_changed_as_node_list(msg) {
        let nodes = []
        for(let i = 0; i < msg.nodesLength(); i++) {
            const n = msg.nodes(i);
            const id = this._id(n.nodeId())
            let node = this._nodes.get(id.toString())
            if(!node) {
                node = new Node(this, id, n.status())
                this._nodes.set(id.toString(), node)
            }
            if(n.status() == Node.Status.disconnected) {
                this._nodes.delete(id.toString())
            }
            nodes.push(node)
            node._set_status(n.status())
        }
        return nodes
    }

    _id(fb_id) {
        return new NodeId(fb_id.idArray())
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

    _create_node_id(builder, id) {
        const offset = mobsya.fb.NodeId.createIdVector(builder, id.data);
        mobsya.fb.NodeId.startNodeId(builder)
        mobsya.fb.NodeId.addId(builder, offset)
        return mobsya.fb.NodeId.endNodeId(builder)
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
            if(req == undefined) {
                console.error(`unknown request ${id}`)
            }
            return req
    }
    _prepare_request(req_id) {
        let req = new Request(req_id)
        this._requests.set(req_id, req)
        return req._promise
    }
}
