import sys
import graphviz as gv
import time

MAX_NUM_NODES = 4

# re-renders the graph file
def render_graph(light_values, node_graph):
	graph = gv.Digraph(comment="Mesh Network", format='png')

	nodes = set(node_graph.keys())
	# hack to flatten list of lists of neighbor ids
	# add add to node set
	nodes.update(sum(node_graph.values(), []))
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

if __name__ == '__main__':
	light_values = {} # map a node to its light sensor value
	node_graph = {} # map a node to a list of its neighbors
	while True:
		inData = sys.stdin.readline()
		if inData:
			print inData
			if "Neighbors" in inData:
				parse_neighbor_line(inData, node_graph)
				print "new graph: ", node_graph
				render_graph(light_values, node_graph)

			elif "value" in inData:
				parse_light_value_line(inData, light_values)
				print "new values: ", light_values
				render_graph(light_values, node_graph)
			if 'end' in inData:
				 break;


