# Custom File System in C

A file system that runs on a simulated block device. It keeps its state on disk
between runs and comes with an interactive shell. Built for CSC 415 (Operating
Systems) at San Francisco State University, Fall 2025.

## Background

This started as a 4-person team project (Ian Kligman, Joe Bowen, Enrique Cabral-Mixco,
Cheng-Chieh "James" Yao). The team's final submission used **contiguous allocation**;
that version is preserved in the `main` branch history.

After the course, I (James) independently replaced contiguous allocation with
**FAT-based linked allocation**. Our team had originally planned this design but
did not complete it. That work is on the `linked-list-allocation` branch.

## Shell commands

`ls` `cd` `pwd` `md` `rm` `touch` `cat` `cp` `mv` `cp2fs` `cp2l`

## On-disk layout (512-byte blocks)

| LBA      | Contents                                   |
|----------|--------------------------------------------|
| 0        | Volume Control Block (VCB)                 |
| 1–5      | Free-space bitmap                          |
| 6        | Root directory                             |
| 7–159    | File Allocation Table (one int per block)  |
| 160+     | File and directory data                    |

## Linked allocation (my extension)

**Problem.** The contiguous allocator handed out blocks from a moving "tail".
Space freed by deleting a file was never reused unless it sat right at the tail.
Repeated create/delete cycles caused external fragmentation, and the volume
eventually filled up even when it had plenty of free blocks.

**Solution.**
- Added a File Allocation Table: `fat[b]` holds the next block after `b`, and
  `FAT_END` (-1) marks the end of a chain. The FAT is stored on disk and loaded
  at startup.
- `fsmAllocateBlock()` allocates first-fit from the bitmap, so freed blocks
  anywhere on the volume can be reused.
- `getFileBlockLBA()` in the I/O layer walks a file's chain to find its n-th block.
  On writes it extends the chain when needed. Every place that used to compute
  `start + offset` now goes through this function.
- Delete frees the whole chain. `O_TRUNC` cuts the chain back and frees the tail.
- `fatSetNext()` writes back only the single FAT block it changed, not the
  whole 153-block table.
- Fixed a latent bug: `touch` never reserved the new file's head block in the
  bitmap. It was harmless under tail allocation, but it would have caused
  double allocation once freed space started being reused.

## Demo: reusing a fragmented gap

```
cp2fs big.txt big1        # big1 occupies blocks 160–187
touch small               # small occupies block 188
rm big1                   # frees 160–187, leaving a gap before small
cp2fs bigger.txt big2     # big2 fills the gap, then skips 188
```

Output (excerpt):
```
DEBUG: touch success! made empty file small at block 188
In fs_delete(): freed 28 blocks for big1
Linked block 160 -> 161
...
Linked block 186 -> 187
Linked block 187 -> 189   <- skips block 188, which belongs to "small"
Linked block 189 -> 190
...
```
Reading `big2` back out with `cp2l` and running `diff` against the original
shows the files are byte-for-byte identical.

Full output: [docs/fragmentation_demo.txt](docs/fragmentation_demo.txt)

## Build and run

Requires Linux (x86-64 or aarch64), `gcc`, `make`, and `libreadline-dev`.

```bash
make run
```

## Limitations and future work

- Reaching the n-th block walks the chain from the head, so a sequential read
  costs O(n²) in total. Caching the last (block index, LBA) pair in the file
  control block would make it O(n).
- Directories are still a single block.
- The bitmap and the FAT overlap in what they track. FAT32 uses the FAT alone
  to mark free blocks.
