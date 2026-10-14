#include <cstdio>
#include <string>
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
    std::unordered_map<uint, std::unordered_set<Blk *>> init_table;
    std::unordered_map<Blk *, std::unordered_set<uint>> blk_defs;

    auto add_def = [&](Blk *blk, Ref r) {
        if (is_variable(r)) {
            init_table[r.val].insert(blk);
            blk_defs[blk].insert(r.val);
        }
    };

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        for (uint i = 0; i < blk->nins; ++i) {
            add_def(blk, blk->ins[i].to);
        }
    }

    std::unordered_map<Blk *, std::unordered_set<std::string>> gen;
    std::unordered_map<Blk *, std::unordered_set<std::string>> kill;

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        for (uint var : blk_defs[blk]) {
            gen[blk].insert("@" + std::string(blk->name) + "%" + fn->tmp[var].name);
            for (Blk *other_blk : init_table[var]) {
                if (other_blk != blk) {
                    kill[blk].insert("@" + std::string(other_blk->name) + "%" + fn->tmp[var].name);
                }
            }
        }
    }

    std::unordered_map<Blk *, std::vector<Blk *>> preds;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        if (blk->s1) preds[blk->s1].push_back(blk);
        if (blk->s2 && blk->s2 != blk->s1) preds[blk->s2].push_back(blk);
    }

    std::unordered_map<Blk *, std::unordered_set<std::string>> in;
    std::unordered_map<Blk *, std::unordered_set<std::string>> out;

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        out[blk] = gen[blk];
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (Blk *blk = fn->start; blk; blk = blk->link) {
            std::unordered_set<std::string> new_in;
            if (blk != fn->start) {
                for (Blk *p : preds[blk]) {
                    new_in.insert(out[p].begin(), out[p].end());
                }
            }

            std::unordered_set<std::string> new_out = gen[blk];
            for (const std::string &d : new_in) {
                if (!kill[blk].count(d)) {
                    new_out.insert(d);
                }
            }

            if (new_in != in[blk] || new_out != out[blk]) {
                in[blk] = std::move(new_in);
                out[blk] = std::move(new_out);
                changed = true;
            }
        }
    }

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        printf("@%s\n", blk->name);
        printf("\trd_in =");
        for (const std::string &d : in[blk]) {
            printf(" %s", d.c_str());
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
