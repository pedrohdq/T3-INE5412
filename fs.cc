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

    union fs_block inode_block;
    for (int i = 0; i < block.super.ninodeblocks; i++) {
        disk->read(i + 1, inode_block.data);
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            if (inode_block.inode[j].isvalid) {
                cout << "inode " << (i * INODES_PER_BLOCK) + j + 1 << ":\n";
                cout << "    size: " << inode_block.inode[j].size << " bytes\n";
                cout << "    direct blocks:";
                for (int direct : inode_block.inode[j].direct) {
                    if (direct) {
                        cout << " " << direct;
                    }
                }
                cout << "\n";
                union fs_block pointer_block;
                if (inode_block.inode[j].indirect) {
                    cout << "    indirect block: " << inode_block.inode[j].indirect << "\n";
                    disk->read(inode_block.inode[j].indirect, pointer_block.data);
                    cout << "    indirect data blocks:";
                    for (int pointer : pointer_block.pointers) {
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
        for (int i = 0; i < block.super.ninodeblocks; i++) {
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
        inode->isvalid = 0;
        inode->size = 0;
        for (int &direct: inode->direct) {
            if (direct) {
                bitmap[direct] = 0;
                direct = 0;
            }
        }
        if (inode->indirect) {
            bitmap[inode->indirect] = 0;
            union fs_block pointer_block;
            disk->read(inode->indirect, pointer_block.data);
            for (int pointer : pointer_block.pointers) {
                bitmap[pointer] = 0;
            }
            inode->indirect = 0;
        }
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
    auto *inode = new fs_inode;
    int bytes_read = 0;
    if (is_disk_mounted && inode_load(inumber, inode)) {
        int block_offset = offset / Disk::DISK_BLOCK_SIZE;
        int offset_in_block = offset % Disk::DISK_BLOCK_SIZE;
        while (bytes_read != length) {
            if (block_offset < POINTERS_PER_INODE) {
                if (inode->direct[block_offset]) {
                    union fs_block block_to_be_read;
                    disk->read(inode->direct[block_offset], block_to_be_read.data);
                    int bytes_to_read = min(length - bytes_read, Disk::DISK_BLOCK_SIZE - offset_in_block);
                    for (int i = 0; i < bytes_to_read; i++) {
                        data[bytes_read + i] = block_to_be_read.data[offset_in_block + i];
                    }
                    bytes_read += bytes_to_read;
                    block_offset++;
                    offset_in_block = 0;
                } else {
                    break;
                }
            } else {
                if (inode->indirect) {
                    union fs_block pointer_block;
                    disk->read(inode->indirect, pointer_block.data);
                    int pointer_offset = block_offset - POINTERS_PER_INODE;
                    if (pointer_block.pointers[pointer_offset]) {
                        union fs_block block_to_be_read;
                        disk->read(pointer_block.pointers[pointer_offset], block_to_be_read.data);
                        int bytes_to_read = min(length - bytes_read, Disk::DISK_BLOCK_SIZE - offset_in_block);
                        for (int i = 0; i < bytes_to_read; i++) {
                            data[bytes_read + i] = block_to_be_read.data[offset_in_block + i];
                        }
                        bytes_read += bytes_to_read;
                        block_offset++;
                        offset_in_block = 0;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
    }
	return bytes_read;
}

int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset)
{
    auto *inode = new fs_inode;
    int bytes_written = 0;
    if (is_disk_mounted && inode_load(inumber, inode)) {
        int block_offset = offset / Disk::DISK_BLOCK_SIZE;
        int offset_in_block = offset % Disk::DISK_BLOCK_SIZE;
        while (bytes_written != length) {
            if (block_offset < POINTERS_PER_INODE) {
                // inicializa um bloco de dados para referencia direta
                // se ainda não existir
                if (!inode->direct[block_offset]) {
                    int block_number = allocate_block();
                    if (block_number == -1) {
                        break;
                    }
                    inode->direct[block_offset] = block_number;
                }

                union fs_block block;
                disk->read(inode->direct[block_offset], block.data);
                int bytes_to_write = min(length - bytes_written, Disk::DISK_BLOCK_SIZE - offset_in_block);
                for (int i = 0; i < bytes_to_write; i++) {
                    block.data[offset_in_block + i] = data[bytes_written + i];
                }
                disk->write(inode->direct[block_offset], block.data);
                bytes_written += bytes_to_write;
                inode->size += bytes_to_write;
                block_offset++;
                offset_in_block = 0;
                inode_save(inumber, inode);
            } else {
                // inicializa um bloco de pointers para referencia indireta
                // se ainda não existir
                if (!inode->indirect) {
                    int block_number = allocate_block();
                    if (block_number == -1) {
                        break;
                    }
                    inode->indirect = block_number;
                }

                // inicializa um bloco de dados para o pointer se ainda não existir
                union fs_block pointer_block;
                disk->read(inode->indirect, pointer_block.data);
                int pointer_offset = block_offset - POINTERS_PER_INODE;
                if (!pointer_block.pointers[pointer_offset]) {
                    int block_number = allocate_block();
                    if (block_number == -1) {
                        break;
                    }
                    pointer_block.pointers[pointer_offset] = block_number;
                }

                union fs_block block;
                disk->read(pointer_block.pointers[pointer_offset], block.data);
                int bytes_to_write = min(length - bytes_written, Disk::DISK_BLOCK_SIZE - offset_in_block);
                for (int i = 0; i < bytes_to_write; i++) {
                    block.data[offset_in_block + i] = data[bytes_written + i];
                }
                disk->write(pointer_block.pointers[pointer_offset], block.data);
                bytes_written += bytes_to_write;
                inode->size += bytes_to_write;
                block_offset++;
                offset_in_block = 0;
                inode_save(inumber, inode);
            }
        }
    }
	return bytes_written;
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

int INE5412_FS::allocate_block() {
    union fs_block superblock;
    int block_number = -1;

    disk->read(0, superblock.data);
    for (int i = 0; i < superblock.super.nblocks; i++) {
        if (!bitmap[i]) {
            bitmap[i] = 1;
            block_number = i;
            sanitaze_block(block_number);
            break;
        }
    }
    return block_number;
}

void INE5412_FS::sanitaze_block(int blocknum) {
    union fs_block block;

    for (char &byte : block.data) {
        byte = 0;
    }
    disk->write(blocknum, block.data);

}
