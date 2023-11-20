#include "fs.h"

int INE5412_FS::fs_format()
{
    // uma tentativa de formatar um disco que já foi montado não deve fazer
    // nada e retornar falha
    if (!is_disk_mounted) {
        union fs_block init_superblock;

        init_superblock.super.magic = FS_MAGIC;
        init_superblock.super.nblocks = disk->size();
        // reserva 10% dos blocos para inodos
        init_superblock.super.ninodeblocks = disk->size() / 10;
        init_superblock.super.ninodes = init_superblock.super.ninodeblocks * INODES_PER_BLOCK;

        // escreve o superbloco
        disk->write(0, init_superblock.data);

        // libera a tabela de inodos
        union fs_block empty_block;
        for (auto & inode : empty_block.inode) {
            inode.isvalid = 0;
        }
        for (int i = 0; i < init_superblock.super.ninodeblocks; i++) {
            disk->write(i + 1, empty_block.data);
        }

        return 1;
    }
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
                cout << "inode " << (i * INODES_PER_BLOCK) + j + 1 << ":\n";
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
    union fs_block block;

    disk->read(0, block.data);

    if (!is_disk_mounted && block.super.magic == FS_MAGIC) {
        this->bitmap = new int[block.super.nblocks];
        is_disk_mounted = true;

        // ocupa os blocos do superbloco e da tabela de inodos
        bitmap[0] = 1;
        for (int i = 0; i <= block.super.ninodeblocks; i++) {
            bitmap[i + 1] = 1;
        }
        return 1;
    }
    return 0;

}

int INE5412_FS::fs_create()
{
    if (is_disk_mounted) {
        union fs_block block;

        disk->read(0, block.data);

        for (int i = 0; i <= block.super.ninodeblocks; i++) {
            union fs_block inode_block;

            disk->read(i + 1, inode_block.data);
            for (int j = 0; j < INODES_PER_BLOCK; j++) {
                fs_inode &inode = inode_block.inode[j];
                if (!inode.isvalid) {
                    inode.isvalid = 1;
                    inode.size = 0;
                    for (int &direct: inode.direct) {
                        direct = 0;
                    }
                    inode.indirect = 0;
                    int inumber = (i * INODES_PER_BLOCK) + j + 1;
                    inode_save(inumber, &inode);
                    return inumber;
                }
            }
        }
    }
	return 0;
}

int INE5412_FS::fs_delete(int inumber)
{
    auto *inode = new fs_inode;
    if (is_disk_mounted && inode_load(inumber, inode)) {
        // TODO: liberar blocos de dados
        inode->isvalid = 0;
        inode->size = 0;
        for (int &direct: inode->direct) {
            direct = 0;
        }
        inode->indirect = 0;
        inode_save(inumber, inode);
        return 1;
    }
	return 0;
}

int INE5412_FS::fs_getsize(int inumber)
{
    auto *inode = new fs_inode;
    if (is_disk_mounted && inode_load(inumber, inode)) {
        return inode->size;
    }
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

int INE5412_FS::inode_load(int inumber, INE5412_FS::fs_inode *inode) {
    union fs_block block;

    disk->read(0, block.data);

    for (int i = 0; i <= block.super.ninodeblocks; i++) {
        union fs_block inode_block;

        disk->read(i + 1, inode_block.data);
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            fs_inode &tmp = inode_block.inode[j];
            if (tmp.isvalid && ((i * INODES_PER_BLOCK) + j + 1 == inumber)) {
                *inode = tmp;
                return 1;
            }
        }
    }
    return 0;
}

void INE5412_FS::inode_save(int inumber, INE5412_FS::fs_inode *inode) {
    union fs_block block;

    disk->read(0, block.data);

    for (int i = 0; i <= block.super.ninodeblocks; i++) {
        union fs_block inode_block;

        disk->read(i + 1, inode_block.data);
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            if ((i * INODES_PER_BLOCK) + j + 1 == inumber) {
                inode_block.inode[j] = *inode;
                break;
            }
        }
        disk->write(i + 1, inode_block.data);
    }
}
