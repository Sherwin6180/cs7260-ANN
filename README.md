## Generate datasets of different distributions
```
make distributions
```

## Run on different distributions
We provide two behaviors to compare with: default, and PQ.
The default approach simply picks a place to write randomly, while the PQ approach picks the optimal place.

The general command is
```
make run MODE={MODE} DISTRIBUTION={DISTRIBUTION}
```
For example, if you want to test the PQ approach on the Zipfian dataset, run:
```
make run MODE=pq DISTRIBUTION=zipfian
```
MODE options:
* pq
* default

DISTRIBUTION options:
* uniform
* zipfian
* latest
* hotspot

## Resources
[Tutorial to emulate NVM on DRAM](https://docs.pmem.io/persistent-memory/getting-started-guide/creating-development-environments/linux-environments/linux-memmap)

[Link to our presentation](https://www.canva.com/design/DAGWlymamH4/bNsZgnY8LqdD13K2v9_28Q/edit?utm_content=DAGWlymamH4&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton)

[Introduction to Product Quantization](https://www.pinecone.io/learn/series/faiss/product-quantization/)