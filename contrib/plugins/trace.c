/*
 * Copyright (C) 2020, euler90h@gmail.com
 *
 * Based on insn.c by Emilio G. Cota <cota@braap.org>.
 *
 * License: GNU GPL, version 2 or later.
 *   See the COPYING file in the top-level directory.
 */
//#include "disas/nanomips.h"
#include <bits/stdint-uintn.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static int log_fd;
static char default_output_path[] = "./trace.bin";
static const char *outputh_path = default_output_path;
typedef struct _Instruction {
    void *haddr;
    uint64_t vaddr;
    uint8_t size;
    uint8_t data[16];
} Instruction;

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);
    size_t i;

    for (i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
        const unsigned char *data = qemu_plugin_insn_data(insn);
        Instruction my_insn = {0};
        bzero(my_insn.data, 16);
        uint8_t *p = (uint8_t *)&my_insn;
        size_t len = qemu_plugin_insn_size(insn);
        my_insn.size = len;
        my_insn.haddr = qemu_plugin_insn_haddr(insn);
        my_insn.vaddr = qemu_plugin_insn_vaddr(insn);
        memcpy(my_insn.data, data, len);
        ssize_t bytes_written = write(log_fd, p, sizeof(Instruction));
        assert(bytes_written != -1);
    }
}
                
static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    close(log_fd);
}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id,
                                           const qemu_info_t *info,
                                           int argc, char **argv)
{
    if (argc && argv != NULL && argv[0] != NULL) {
        outputh_path = argv[0];
    }

    log_fd = open(outputh_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    assert(log_fd != -1);
    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    return 0;
}
