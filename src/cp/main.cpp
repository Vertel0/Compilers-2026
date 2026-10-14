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

struct CopyStmt {
    int id;
    Ins *ins;
    Blk *blk;
    uint dst;
    uint src;
};

static void readfn(Fn *fn) {
    fillpreds(fn);
    fillrpo(fn);

    std::vector<CopyStmt> copies;
    std::unordered_map<Ins *, int> ins_to_copy;

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        for (uint i = 0; i < blk->nins; ++i) {
            Ins &ins = blk->ins[i];
            if (ins.op == Ocopy && is_variable(ins.to) && is_variable(ins.arg[0])) {
                int id = (int)copies.size();
                ins_to_copy[&ins] = id;
                copies.push_back({id, &ins, blk, (uint)ins.to.val, (uint)ins.arg[0].val});
            }
        }
    }

    int ncopies = (int)copies.size();

    std::unordered_map<Blk *, std::vector<Blk *>> preds;
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        if (blk->s1) preds[blk->s1].push_back(blk);
        if (blk->s2 && blk->s2 != blk->s1) preds[blk->s2].push_back(blk);
    }

    auto simulate = [&](Blk *blk, const std::unordered_set<int> &in_set, bool apply) {
        std::unordered_set<int> cur = in_set;

        auto resolve = [&](uint val) {
            std::unordered_set<uint> visited;
            while (visited.insert(val).second) {
                bool found = false;
                for (int c : cur) {
                    if (copies[c].dst == val && copies[c].src != val) {
                        val = copies[c].src;
                        found = true;
                        break;
                    }
                }
                if (!found) break;
            }
            return val;
        };

        for (uint i = 0; i < blk->nins; ++i) {
            Ins &ins = blk->ins[i];

            if (apply) {
                for (int a = 0; a < 2; ++a) {
                    if (is_variable(ins.arg[a])) {
                        uint orig = ins.arg[a].val;
                        uint resolved = resolve(orig);
                        if (resolved != orig) {
                            ins.arg[a] = TMP(resolved);
                        }
                    }
                }
            }

            if (ins.op == Ocopy && is_variable(ins.to) && is_variable(ins.arg[0])) {
                int id = ins_to_copy[&ins];
                uint resolved_src = resolve(ins.arg[0].val);
                copies[id].src = resolved_src;

                uint dst = ins.to.val;
                for (auto it = cur.begin(); it != cur.end();) {
                    int c = *it;
                    if (copies[c].dst == dst || copies[c].src == dst) {
                        it = cur.erase(it);
                    } else {
                        ++it;
                    }
                }
                if (dst != resolved_src) {
                    cur.insert(id);
                }
            } else if (is_variable(ins.to)) {
                uint dst = ins.to.val;
                for (auto it = cur.begin(); it != cur.end();) {
                    int c = *it;
                    if (copies[c].dst == dst || copies[c].src == dst) {
                        it = cur.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        if (apply && is_variable(blk->jmp.arg)) {
            uint orig = blk->jmp.arg.val;
            uint resolved = resolve(orig);
            if (resolved != orig) {
                blk->jmp.arg = TMP(resolved);
            }
        }

        return cur;
    };

    std::unordered_set<int> all_copies_set;
    for (int i = 0; i < ncopies; ++i) all_copies_set.insert(i);

    std::unordered_map<Blk *, std::unordered_set<int>> in;
    std::unordered_map<Blk *, std::unordered_set<int>> out;

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        if (blk == fn->start) {
            out[blk] = simulate(blk, {}, false);
        } else {
            out[blk] = all_copies_set;
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < fn->nblk; ++i) {
            Blk *blk = fn->rpo[i];
            if (blk == fn->start) continue;

            std::unordered_set<int> new_in;
            bool first = true;
            for (Blk *p : preds[blk]) {
                if (first) {
                    new_in = out[p];
                    first = false;
                } else {
                    std::unordered_set<int> inter;
                    for (int c : new_in) {
                        if (out[p].count(c)) {
                            inter.insert(c);
                        }
                    }
                    new_in = std::move(inter);
                }
            }

            std::unordered_set<int> new_out = simulate(blk, new_in, false);

            if (new_in != in[blk] || new_out != out[blk]) {
                in[blk] = std::move(new_in);
                out[blk] = std::move(new_out);
                changed = true;
            }
        }
    }

    for (Blk *blk = fn->start; blk; blk = blk->link) {
        simulate(blk, in[blk], true);
    }

    printfn(fn, stdout);
}

static void readdat(Dat *dat) { (void)dat; }

int main() {
    parse(stdin, const_cast<char *>("<stdin>"), readdat, readfn);
    freeall();
    return 0;
}
