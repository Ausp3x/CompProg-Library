"""MST forests and union trees against independent finite oracles.

Quick exhausts undirected looped simple graphs through n=3, adds 40 random
multigraphs, and a 1000-vertex chain. Full uses n=4, 400 random graphs, and a
100000-vertex chain; stress adds 2500 random graphs and a 300000-vertex chain.
Every small graph uses forest-subset enumeration, minimax/maximin Floyd and
threshold transitive closure. Every mode checks that the union tree accepts
n = 2^30 (a bad_alloc under a 1 GiB address-space limit, not under ASan).
Full/stress add ASan/UBSan through the driver.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-mst', ['directed-kruskal', 'directed-prim-sparse',
        'directed-prim-dense', 'directed-boruvka', 'directed-reconstruction',
        'too-many-nodes', 'bottleneck-negative', 'bottleneck-end', 'bottleneck-first-end',
        'bottleneck-past', 'bottleneck-first-past', 'lca-negative', 'lca-end', 'lca-first-end',
        'lca-past', 'lca-first-past', 'threshold-negative', 'threshold-end', 'leaf-range-negative',
        'leaf-range-end', 'leaf-range-past']))
