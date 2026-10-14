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
    std::unordered_map<Blk *, std::unordered_set<uint>> gen_table;
    std::unordered_map<Blk *, std::unordered_set<std::string>> kill_table;

    auto add_def = [&](Blk *blk, Ref r) {
        if (is_variable(r)) {
            init_table[r.val].insert(blk);
            gen_table[blk].insert(r.val);
        }
    };

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        for (uint i = 0; i < blk->nins; ++i) {
            add_def(blk, blk->ins[i].to);
        }
    }

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        for (uint var : gen_table[blk]) {
            for (Blk *other_blk : init_table[var]) {
                if (other_blk != blk) {
                    std::string def_name = "@" + std::string(other_blk->name) + "%" + fn->tmp[var].name;
                    kill_table[blk].insert(def_name);
                }
            }
        }
    }

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        printf("@%s\n", blk->name);

        printf("\tgen =");
        for (uint var : gen_table[blk]) {
            printf(" @%s%%%s", blk->name, fn->tmp[var].name);
        }
        printf("\n");

        printf("\tkill =");
        for (const std::string &def_name : kill_table[blk]) {
            printf(" %s", def_name.c_str());
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
