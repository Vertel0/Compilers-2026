#include <cstdio>
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
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        std::unordered_set<uint> def_set;
        std::unordered_set<uint> use_set;

        auto add_use = [&](Ref r) {
            if (is_variable(r) && !def_set.count(r.val)) {
                use_set.insert(r.val);
            }
        };

        auto add_def = [&](Ref r) {
            if (is_variable(r)) {
              def_set.insert(r.val);
            }
        };

        for (uint i = 0; i < blk->nins; ++i) {
            Ins &ins = blk->ins[i];
            add_use(ins.arg[0]);
            add_use(ins.arg[1]);
            add_def(ins.to);
        }

        add_use(blk->jmp.arg);

        printf("@%s\n", blk->name);
        printf("\tdef =");
        for (uint val : def_set) {
            printf(" %%%s", fn->tmp[val].name);
        }
        printf("\n\tuse =");
        for (uint val : use_set) {
            printf(" %%%s", fn->tmp[val].name);
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
