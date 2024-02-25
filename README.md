# Network Messaging Application

This project is a network messaging application implemented in C. It facilitates message exchange between nodes within a network. The routing mechanism employed is shortest path routing.

## Features

- **Message Exchange**: Nodes within the network can exchange messages seamlessly.
  
- **Shortest Path Routing**: The application utilizes a shortest path routing protocol.
  
- **Routing Table Management**: Each node maintains a routing table, used to calculate the forwarding table. The forwarding table indicates, for each destination, the closest neighboring node.
  
- **Dynamic Network Topology**: The network is based on a ring that connects all nodes. Additionally, cords can be added to this ring.
  
- **Topology Management**: The topology protocol ensures the integrity of the ring upon the addition or removal of nodes.

## Functionality

- **Message Exchange**: Enables nodes to send and receive messages within the network.
  
- **Routing**: Determines the shortest path for message routing based on the maintained routing tables.
  
- **Topology Maintenance**: Handles the addition and removal of nodes while preserving the integrity of the network ring.
  
## Implementation

- **Language**: C
  
- **Routing Algorithm**: Shortest Path Routing
  
- **Topology Protocol**: Ring-based with dynamic node addition and removal support.
  
- **Data Structures**: Utilizes routing tables and forwarding tables for efficient message routing.

## Usage

- The application can be deployed in any network environment where message exchange between nodes is required.
  
- Nodes need to be configured with appropriate routing tables to enable effective communication.

## Future Improvements

- **Optimized Routing Algorithms**: Explore more efficient routing algorithms to improve performance.
  
- **Enhanced Topology Protocol**: Enhance the topology management protocol for better scalability and fault tolerance.
  
- **Security Features**: Implement security measures to safeguard message exchange within the network.

This README provides an overview of the network messaging application, including its features, functionality, implementation details, and potential areas for improvement.
