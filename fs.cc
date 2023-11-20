#include "fs.h"

int INE5412_FS::fs_format()
{
	return 0;
}

void INE5412_FS::fs_debug()
{
	union fs_block block;

	disk->read(0, block.data);

	cout << "superblock:\n";
	cout << "    " << (block.super.magic == FS_MAGIC ? "magic number is valid\n" : "magic number is invalid!\n");
 	cout << "    " << block.super.nblocks << " blocks\n";
	cout << "    " << block.super.ninodeblocks << " inode blocks\n";
	cout << "    " << block.super.ninodes << " inodes\n";

    for (int i = 0; i <= block.super.ninodeblocks; i++) {
        disk->read(i + 1, block.data);
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            if (block.inode[j].isvalid) {
                cout << "inode " << (i * INODES_PER_BLOCK) + j << ":\n";
                cout << "    size: " << block.inode[j].size << " bytes\n";
                cout << "    direct blocks:";
                for (int direct : block.inode[j].direct) {
                    if (direct) {
                        cout << " " << direct;
                    }
                }
                cout << "\n";
                if (block.inode[j].indirect) {
                    cout << "    indirect block: " << block.inode[j].indirect << "\n";
                    disk->read(block.inode[j].indirect, block.data);
                    cout << "    indirect data blocks:";
                    for (int pointer : block.pointers) {
                        if (pointer) {
                            cout << " " << pointer;
                        }
                    }
                    cout << "\n";
                }
            }
        }
    }
}

int INE5412_FS::fs_mount()
{
	return 0;
}

int INE5412_FS::fs_create()
{
	return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
	return 0;
}

int INE5412_FS::fs_getsize(int inumber)
{
	return -1;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset)
{
	return 0;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
	return 0;
}
