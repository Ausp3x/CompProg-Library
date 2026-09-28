"""MST forests and union trees against independent finite oracles.

Quick exhausts undirected looped simple graphs through n=3, adds 40 random
multigraphs, and a 1000-vertex chain. Full uses n=4, 400 random graphs, and a
100000-vertex chain; stress adds 2500 random graphs and a 300000-vertex chain.
Every small graph uses forest-subset enumeration, minimax/maximin Floyd and
threshold transitive closure. Full/stress add ASan/UBSan through the driver.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-mst', ['directed-kruskal', 'directed-prim-sparse',
        'directed-prim-dense', 'directed-boruvka', 'directed-reconstruction',
        'too-many-nodes', 'bottleneck-negative', 'bottleneck-end', 'lca-negative',
        'lca-end', 'threshold-negative', 'threshold-end']))
