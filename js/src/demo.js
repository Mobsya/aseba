import {Client,Node,Request} from './thymio'

//Connect to the switch
//We will need some way to get that url, via the launcher
let client = new Client("ws://localhost:8597");
let selectedNode = undefined

// Start monitotring for node event
// A node will have the state
//      * connected    : Connected but vm description unavailable - little can be done in this state
//      * ready        : The node is ready, we can start communicating with it
//      * busy         : The node is locked by someone else. NB : we will receive busy event for nodes we have a lock on
//      * disconnected : The node is gone
client.on_nodes_changed = async (nodes) => {
    //Iterate over the nodes
    for (let node of nodes) {
        console.log(`${node.id} : ${node.status_str}`)
        // Select the first non busy node
        if((!selectedNode || selectedNode.disconnected) && node.status == Node.Status.ready) {
            selectedNode = node
            try {
                console.log(`Locking ${node.id}`)
                // Lock (take ownership) of the node. We cannot mutate a node (send code to it), until we have a lock on it
                // Once locked, a node will appear busy / unavailable to other clients until we close the connection or call `unlock` explicitely
                // We can lock as many nodes as we want
                await selectedNode.lock();
                console.log("Node locked, sending code")

                // Load some aseba code on the device
                // The code will be compiled on the switch
                await selectedNode.send_aseba_program(
                    `call leds.bottom.left(0,0,0)
                     call leds.bottom.right(32,32,32)
                     call leds.top(0, 32, 0)
                    `)

                // Execute whatever code is loaded on the device
                await selectedNode.run_aseba_program()

            } catch(err) {
                console.log(err)
                switch(err) {
                    case Request.ErrorType.node_busy:
                        console.log("Node Busy !")
                        break
                    default:
                        console.log("unknown error")
                }
            }
            break;
        }
    }
}
