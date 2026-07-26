# FNR (Fluctuating Network Repair)

The source code of paper "*FNR: A Probabilistic Data Repair Scheme for Reed-Solomon Coded Storage Amid Network Fluctuations*"

## Abstract

Reed-Solomon (RS) Code is extensively deployed in large-scale distributed storage systems to provide high data reliability. However, existing RS-based repair methods typically rely on a deterministic assumption of constant cross-rack available bandwidth, failing to account for the stochastic network dynamics inherent in production environments. Our empirical evaluations, spanning over 11 months, reveal significant throughput fluctuations, likely induced by multi-tenant contention and background traffic. Neglecting these dynamics inevitably leads to severe performance degradation and even repair failures. To address this issue, we propose **F**luctuating **N**etwork **R**epair (**FNR**), a repair scheme tailored for volatile network conditions. FNR redefines the network throughput model with a probabilistic framework characterized by (𝑐𝑜𝑠𝑡, 𝑝𝑟𝑜𝑏𝑎𝑏𝑖𝑙𝑖𝑡𝑦) pairs. By seamlessly integrating this model into state-of-the-art (SOTA) repair methods, FNR expands a larger search space to identify better repair solutions. Experimental results demonstrate that in soft real-time scenarios, FNR reduces repair time by up to 17.49%, 22.79%, and 22.41% compared to Random Repair (RR), AZ-Recovery (AZ), and Computation Time Priority (CTP), respectively. Furthermore, in hard real-time scenarios, FNR provides robust deadline guarantees where SOTA methods falter.

## File Structure

```
FNR/
├── Compare.h                              # Header file
├── Compare_All.c                          # Main program entry
├── Compare_Func.c                         # Core implementation
├── Compare_config.txt                     # Configuration file for parameters
├── Fixed_RackTransfer-RACK_NUM.txt        # Inter-rack transmission costs for fixed mode
├── Fluctuate_RackTransfer-RACK_NUM.txt    # Inter-rack transmission cost and probability distributions for fluctuating mode
├── LICENSE                                # Build script
├── Makefile                               # Open-source license file
└── README.md                              # Project documentation
```

## Configuration

### System Parameters

Edit `Compare_config.txt` to customize the system parameters.

| Parameter                   | Description                            |
| :-------------------------- | -------------------------------------- |
| DATA_BLOCK_NUM_PER_STRIPE   | Number of data blocks per stripe (k)   |
| PARITY_BLOCK_NUM_PER_STRIPE | Number of parity blocks per stripe (m) |
| STRIPE_NUM                  | Number of stripes                      |
| NODE_NUM_PER_RACK           | Number of nodes per rack               |
| RACK_NUM                    | Number of racks                        |

### Inter-rack Transmission Cost Files

Three rack-scale configurations (5 / 20 / 100 racks) are provided in two categories:

1. **Fixed-cost files** (`Fixed_RackTransfer-RACK_NUM.txt`)

   Each line follows the format `fromRack toRack cost`, representing the deterministic transmission cost between two racks.

2. **Fluctuating-cost files** (`Fluctuate_RackTransfer-RACK_NUM.txt`)

   The header line follows `fromRack-toRack count:`, followed by lines in `[cost-prob]` format. Each rack pair has `count` possible transmission costs with corresponding occurrence probabilities, simulating network bandwidth fluctuation.

> **Note:** You must update the filenames passed to `read_fixed_rack_transfer()` and `read_fluctuate_rack_transfer()` in `Compare_ALL.c` to match the configured `RACK_NUM`, otherwise array out-of-bounds errors will occur.

## Build & Run

### Build

Option 1: Use Makefile

```bash
make
```

Option 2: Manual compilation

```bash
gcc Compare_ALL.c -o Compare_ALL -lm -O2
```

### Run

```bash
./Compare_ALL
```

## Citation

If you use this code in your research, please cite our paper:

```bibtex
@inproceedings{FNR2026,
  title={FNR: A Probabilistic Data Repair Scheme for Reed-Solomon Coded Storage Amid Network Fluctuations},
  author={Huizhao Feng and Yikun Hu and Zhen Luo and Tao Lu and Huizhang Luo and Kenli Li},
  booktitle = {Proc. of ICPP},
  year={2026},
  doi={10.1145/3832810.3832880},
}
```

## License

This project is licensed under the MIT License. See `LICENSE` file for details.

