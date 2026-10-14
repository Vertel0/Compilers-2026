#ifdef __cplusplus
#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export
#else
#include <qbe/all.h>
#endif

#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>

static void readfn(Fn *fn) {
  fillrpo(fn);
  fillpreds(fn);
  filluse(fn);
  ssa(fn);

  std::vector<Blk *> blks;
  std::unordered_map<Blk *, int> blk_to_id;
  for (Blk *b = fn->start; b; b = b->link) {
    blk_to_id[b] = static_cast<int>(blks.size());
    blks.push_back(b);
  }
  int n = static_cast<int>(blks.size());
  int exit_id = n;

  std::vector<std::vector<int>> succs(n);
  for (int i = 0; i < n; ++i) {
    Blk *b = blks[i];
    if (isret(b->jmp.type)) {
      succs[i].push_back(exit_id);
    } else {
      if (b->s1) succs[i].push_back(blk_to_id[b->s1]);
      if (b->s2 && b->s2 != b->s1) succs[i].push_back(blk_to_id[b->s2]);
      if (succs[i].empty()) {
        succs[i].push_back(exit_id);
      }
    }
  }

  std::vector<std::vector<bool>> pdom(n + 1, std::vector<bool>(n + 1, true));
  pdom[exit_id].assign(n + 1, false);
  pdom[exit_id][exit_id] = true;

  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = 0; i < n; ++i) {
      std::vector<bool> new_p(n + 1, false);
      if (!succs[i].empty()) {
        new_p = pdom[succs[i][0]];
        for (size_t k = 1; k < succs[i].size(); ++k) {
          int s = succs[i][k];
          for (int j = 0; j <= n; ++j) {
            if (!pdom[s][j]) new_p[j] = false;
          }
        }
      }
      new_p[i] = true;
      if (new_p != pdom[i]) {
        pdom[i] = new_p;
        changed = true;
      }
    }
  }

  std::vector<int> ipdom(n, -1);
  for (int i = 0; i < n; ++i) {
    int best = -1;
    int max_sz = -1;
    for (int j = 0; j <= n; ++j) {
      if (j != i && pdom[i][j]) {
        int sz = 0;
        for (int k = 0; k <= n; ++k) {
          if (pdom[j][k]) sz++;
        }
        if (sz > max_sz) {
          max_sz = sz;
          best = j;
        }
      }
    }
    ipdom[i] = best;
  }

  std::vector<std::vector<int>> cd_parents(n);
  for (int a = 0; a < n; ++a) {
    for (int s : succs[a]) {
      if (s == exit_id) continue;
      if (s == a || !pdom[a][s]) {
        int curr = s;
        int target = ipdom[a];
        while (curr != target && curr != exit_id && curr != -1) {
          cd_parents[curr].push_back(a);
          curr = ipdom[curr];
        }
      }
    }
  }

  std::unordered_map<int, Ins *> def_ins;
  std::unordered_map<int, Phi *> def_phi;
  std::unordered_map<Ins *, Blk *> ins_blk;
  std::unordered_map<Phi *, Blk *> phi_blk;

  for (Blk *b : blks) {
    for (Phi *p = b->phi; p; p = p->link) {
      if (rtype(p->to) == RTmp && p->to.val >= Tmp0) {
        def_phi[p->to.val] = p;
        phi_blk[p] = b;
      }
    }
    for (Ins *i = b->ins; i < &b->ins[b->nins]; ++i) {
      if (req(i->to, R) == 0 && rtype(i->to) == RTmp && i->to.val >= Tmp0) {
        def_ins[i->to.val] = i;
        ins_blk[i] = b;
      }
    }
  }

  std::unordered_set<Ins *> marked_ins;
  std::unordered_set<Phi *> marked_phi;
  std::unordered_set<Blk *> marked_jmp;
  std::unordered_set<int> marked_blks;
  std::queue<int> wl;

  auto mark_block = [&](auto &self, int b_id) -> void {
    if (b_id < 0 || b_id >= n) return;
    if (marked_blks.count(b_id)) return;
    marked_blks.insert(b_id);

    for (int p_id : cd_parents[b_id]) {
      Blk *pb = blks[p_id];
      if (!marked_jmp.count(pb)) {
        marked_jmp.insert(pb);
        if (pb->jmp.type == Jjnz) {
          if (rtype(pb->jmp.arg) == RTmp && pb->jmp.arg.val >= Tmp0) {
            wl.push(pb->jmp.arg.val);
          }
        }
        self(self, p_id);
      }
    }
  };

  for (int i = 0; i < n; ++i) {
    Blk *b = blks[i];
    if (isret(b->jmp.type)) {
      marked_jmp.insert(b);
      mark_block(mark_block, i);
      if (rtype(b->jmp.arg) == RTmp && b->jmp.arg.val >= Tmp0) {
        wl.push(b->jmp.arg.val);
      }
    }
    for (Ins *ins = b->ins; ins < &b->ins[b->nins]; ++ins) {
      if (isstore(ins->op) || ins->op == Ocall || ins->op == Ovacall || isarg(ins->op)) {
        marked_ins.insert(ins);
        mark_block(mark_block, i);
        if (rtype(ins->arg[0]) == RTmp && ins->arg[0].val >= Tmp0) {
          wl.push(ins->arg[0].val);
        }
        if (rtype(ins->arg[1]) == RTmp && ins->arg[1].val >= Tmp0) {
          wl.push(ins->arg[1].val);
        }
      }
    }
  }

  while (!wl.empty()) {
    int v = wl.front();
    wl.pop();

    auto it_ins = def_ins.find(v);
    if (it_ins != def_ins.end()) {
      Ins *ins = it_ins->second;
      if (!marked_ins.count(ins)) {
        marked_ins.insert(ins);
        mark_block(mark_block, blk_to_id[ins_blk[ins]]);
        if (rtype(ins->arg[0]) == RTmp && ins->arg[0].val >= Tmp0) {
          wl.push(ins->arg[0].val);
        }
        if (rtype(ins->arg[1]) == RTmp && ins->arg[1].val >= Tmp0) {
          wl.push(ins->arg[1].val);
        }
      }
    }

    auto it_phi = def_phi.find(v);
    if (it_phi != def_phi.end()) {
      Phi *phi = it_phi->second;
      if (!marked_phi.count(phi)) {
        marked_phi.insert(phi);
        mark_block(mark_block, blk_to_id[phi_blk[phi]]);
        for (uint k = 0; k < phi->narg; ++k) {
          if (rtype(phi->arg[k]) == RTmp && phi->arg[k].val >= Tmp0) {
            wl.push(phi->arg[k].val);
          }
        }
      }
    }
  }

  auto is_useful = [&](int b_id) -> bool {
    if (b_id < 0 || b_id >= n) return false;
    Blk *b = blks[b_id];
    if (marked_jmp.count(b)) return true;
    for (Ins *ins = b->ins; ins < &b->ins[b->nins]; ++ins) {
      if (marked_ins.count(ins)) return true;
    }
    for (Phi *p = b->phi; p; p = p->link) {
      if (marked_phi.count(p)) return true;
    }
    return false;
  };

  for (int i = 0; i < n; ++i) {
    Blk *b = blks[i];
    for (Ins *ins = b->ins; ins < &b->ins[b->nins]; ++ins) {
      if (!marked_ins.count(ins)) {
        ins->op = Onop;
        ins->to = R;
        ins->arg[0] = R;
        ins->arg[1] = R;
      }
    }

    Phi **pp = &b->phi;
    while (*pp) {
      if (!marked_phi.count(*pp)) {
        *pp = (*pp)->link;
      } else {
        pp = &(*pp)->link;
      }
    }

    if (b->jmp.type == Jjnz && !marked_jmp.count(b)) {
      int target = ipdom[i];
      while (target >= 0 && target < n && !is_useful(target)) {
        target = ipdom[target];
      }
      if (target < 0 || target >= n) {
        target = ipdom[i];
      }
      if (target >= 0 && target < n) {
        b->jmp.type = Jjmp;
        b->jmp.arg = R;
        b->s1 = blks[target];
        b->s2 = nullptr;
      }
    }
  }

  fillpreds(fn);
  fillrpo(fn);
  printfn(fn, stdout);
}

static void readdat(Dat *dat) { (void)dat; }

int main() {
  parse(stdin, const_cast<char *>("<stdin>"), readdat, readfn);
  freeall();
  return 0;
}
