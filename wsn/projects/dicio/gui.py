import sys
import time
import graphviz as gv
import serial

class MSG_TYPE:
	NODE_SENSOR_VALUE, NODE_NEIGHBORS, GATEWAY2NODE = range(3)

MAX_NUM_NEIGHBORS = 3

## NEIGHBOR MAP DEFINITION ##
# [node:[num_neighbors][neighbor_list]]
# [node:[num_neighbors][neighbor_list]]
# ...

# Given a string input from the serial port, parse into a packet.
# Output should be the updated node graph
# Format: "   Origin: %d -> Neighbors" %d %d %d"
def parse_neighbor_line(line, node_graph):
	words = line.split(' ')
	# trim empty spaces
	words = [word for word in words if len(word) > 0]
	if len(words) < 7:
		print "Error: message not long enough"
		print words
		return
	origin_id = int(words[1])
	neighbors = [int(id) for id in words[4:]]
	neighbors = [neighbor_id for neighbor_id in neighbors if neighbor_id != 0]
	node_graph[origin_id] = neighbors
	print node_graph

def parse_light_value_line(line, light_values):
	words = line.split(' ')
	# trim empty spaces
	words = [word for word in words if len(word) > 0]
	if len(words) < 5:
		print "Error: message not long enough"
		print words
		return
	origin_id = int(words[2].replace(',',''))
	value = int(words[4])
	light_values[origin_id] = value


# re-renders the graph file
def render_graph(light_values, node_graph):
	graph = gv.Digraph(comment="Mesh Network", format='png')

	# build a node list based on light value keys,
	# neighbor graph keys, and neighborgraph lists
	nodes = set(node_graph.keys())
	# hack to flatten list of lists of neighbor ids
	node_graph_neighbors = sum(node_graph.values(), [])
	nodes.update(node_graph_neighbors)
	nodes.update(light_values.keys())
	print "nodes: ", nodes
	for node_id in nodes:
		value = 0
		if node_id in light_values:
			value = light_values[node_id]
		graph.node(str(node_id), str(value))

	for node_id, neighbors in node_graph.iteritems():
		for neighbor_id in neighbors:
			graph.edge(str(node_id), str(neighbor_id))


	GRAPH_FILE = 'graph.png'
	print graph.source
	graph.render(GRAPH_FILE, view=True)


if __name__ == '__main__':
	if len(sys.argv) < 2:
		print "Error: must pass in serial port as command line arg"
		sys.exit(1)
	light_values = {} # map a node to its light sensor value
	node_graph = {} # map a node to a list of its neighbors

	SERIAL_PORT = sys.argv[1]
	print "Serial Port: ", SERIAL_PORT

	SP = serial.Serial(SERIAL_PORT, 115200)

	if SP.isOpen():
		while True:
			# pipe serial output to stdout
			serial_line = SP.readline()
			if serial_line:
				print serial_line
				if "Neighbors:" in serial_line:
					parse_neighbor_line(line, node_graph)
					render_graph(light_values, node_graph)
				elif "value:" in serial_line:
					parse_light_value_line(serial_line, light_values)
					render_graph(light_values, node_graph)
			#input_line = sys.stdin.readline()
			#if input_line:
				#SP.write(input_line)
	else:
		print "Error: unable to open serial port"
		sys.exit(1)
