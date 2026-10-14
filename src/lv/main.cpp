#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef __cplusplus
#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export
#else
#include <qbe/all.h>
#endif

static bool is_variable(Ref r) {
    return rtype(r) == RTmp && r.val >= Tmp0;
}

static void readfn(Fn *fn) {
    fillpreds(fn);
    fillrpo(fn);

    std::unordered_map<Blk *, std::unordered_set<uint>> blk_def;
    std::unordered_map<Blk *, std::unordered_set<uint>> blk_use;

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        auto add_use = [&](Ref r) {
            if (is_variable(r) && !blk_def[blk].count(r.val)) {
                blk_use[blk].insert(r.val);
            }
        };

        auto add_def = [&](Ref r) {
            if (is_variable(r)) {
                blk_def[blk].insert(r.val);
            }
        };

        for (uint i = 0; i < blk->nins; ++i) {
            Ins &ins = blk->ins[i];
            add_use(ins.arg[0]);
            add_use(ins.arg[1]);
            add_def(ins.to);
        }

        add_use(blk->jmp.arg);
    }

    std::unordered_map<Blk *, std::unordered_set<uint>> in;
    std::unordered_map<Blk *, std::unordered_set<uint>> out;

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = fn->nblk - 1; i >= 0; --i) {
            Blk *blk = fn->rpo[i];

            std::unordered_set<uint> new_out;
            if (blk->s1) {
                new_out.insert(in[blk->s1].begin(), in[blk->s1].end());
            }
            if (blk->s2) {
                new_out.insert(in[blk->s2].begin(), in[blk->s2].end());
            }

            std::unordered_set<uint> new_in = blk_use[blk];
            for (uint v : new_out) {
                if (!blk_def[blk].count(v)) {
                    new_in.insert(v);
                }
            }

            if (new_out != out[blk] || new_in != in[blk]) {
                out[blk] = std::move(new_out);
                in[blk] = std::move(new_in);
                changed = true;
            }
        }
    }

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        printf("@%s\n", blk->name);
        printf("\tlv_out =");
        for (uint var : out[blk]) {
            printf(" %%%s", fn->tmp[var].name);
        }
        printf("\n\n");
    }
}

static void readdat(Dat *dat) { (void)dat; }

int main() {
    parse(stdin, const_cast<char *>("<stdin>"), readdat, readfn);
    freeall();
    return 0;
}
