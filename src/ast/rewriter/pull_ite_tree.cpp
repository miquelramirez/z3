/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    pull_ite_tree.cpp

Abstract:

    <abstract>

Author:

    Leonardo de Moura (leonardo) 2008-06-22.

Revision History:

--*/
#include "ast/rewriter/pull_ite_tree.h"
#include "ast/recurse_expr_def.h"
#include "ast/for_each_expr.h"
#include "ast/ast_pp.h"

pull_ite_tree::pull_ite_tree(ast_manager & m):
    m_manager(m),
    m_rewriter(m),
    m_cache(m) {
}

void pull_ite_tree::cache_result(expr * n, expr * r, proof * pr) { 
    m_cache.insert(n, r, pr); 
}

void pull_ite_tree::visit(expr * n, bool & visited) {
    if (!is_cached(n)) {
        m_todo.push_back(n);
        visited = false;
    }
}

bool pull_ite_tree::visit_children(expr * n) {
    if (m_manager.is_ite(n)) {
        bool visited = true;
        visit(to_app(n)->get_arg(1), visited);
        visit(to_app(n)->get_arg(2), visited);
        return visited;
    }
    else {
        return true;
    }
}

void pull_ite_tree::reduce(expr * n) {
    // Remark: invoking the simplifier to build the new expression saves a lot of memory.
    if (m_manager.is_ite(n)) {
        expr * c = to_app(n)->get_arg(0);
        expr * t_old = to_app(n)->get_arg(1);
        expr * e_old = to_app(n)->get_arg(2);
        expr * t     = nullptr;
        proof * t_pr = nullptr;
        expr * e     = nullptr;
        proof * e_pr = nullptr;
        get_cached(t_old, t, t_pr);
        get_cached(e_old, e, e_pr);
        expr_ref r(m_manager);
        expr * args[3] = {c, t, e};
        r = m_rewriter.mk_app(to_app(n)->get_decl(), 3, args);
        if (!m_manager.proofs_enabled()) {
            // expr * r = m_manager.mk_ite(c, t, e);
            cache_result(n, r, nullptr);
        }
        else {
            // t_pr is a proof for (m_p ... t_old ...) == t
            // e_pr is a proof for (m_p ... e_old ...) == e
            expr_ref old(m_manager);
            expr_ref p_t_old(m_manager);
            expr_ref p_e_old(m_manager);
            old     = mk_p_arg(n); // (m_p ... n ...) where n is (ite c t_old e_old)
            p_t_old = mk_p_arg(t_old); // (m_p ... t_old ...)
            p_e_old = mk_p_arg(e_old); // (m_p ... e_old ...)
            expr_ref tmp1(m_manager);
            tmp1 = m_manager.mk_ite(c, p_t_old, p_e_old); // (ite c (m_p ... t_old ...) (m_p ... e_old ...))
            proof * pr1 = m_manager.mk_rewrite(old, tmp1);  // proof for (m_p ... (ite c t_old e_old) ...) = (ite c (m_p ... t_old ...) (m_p ... e_old ...))
            expr_ref tmp2(m_manager);
            tmp2 = m_manager.mk_ite(c, t, e); // (ite c t e)
            proof * pr2 = nullptr; // it will contain a proof for (ite c (m_p ... t_old ...) (m_p ... e_old ...)) = (ite c t e)
            proof * pr3 = nullptr; // it will contain a proof for (m_p ... (ite c t_old e_old) ...)               = (ite c t e)
            proof * proofs[2];
            unsigned num_proofs = 0;
            if (t_pr != nullptr) {
                proofs[num_proofs] = t_pr;
                num_proofs++;
            }
            if (e_pr != nullptr) {
                proofs[num_proofs] = e_pr;
                num_proofs++;
            }
            if (num_proofs > 0) {
                pr2 = m_manager.mk_congruence(to_app(tmp1), to_app(tmp2), num_proofs, proofs);
                pr3 = m_manager.mk_transitivity(pr1, pr2);
            }
            else {
                pr3 = pr1;
            }
            proof * pr4 = nullptr; // it will contain a proof for (ite c t e) = r
            proof * pr5 = nullptr; // it will contain a proof for (m_p ... (ite c t_old e_old) ...) = r
            if (tmp2 != r) {
                pr4 = m_manager.mk_rewrite(tmp2, r);
                pr5 = m_manager.mk_transitivity(pr3, pr4);
            }
            else {
                pr5 = pr3;
            }
            cache_result(n, r, pr5);
        }
    }
    else {
        expr_ref r(m_manager);
        m_args[m_arg_idx] = n;
        r = m_rewriter.mk_app(m_p, m_args.size(), m_args.c_ptr());
        if (!m_manager.proofs_enabled()) {
            // expr * r = m_manager.mk_app(m_p, m_args.size(), m_args.c_ptr());
            cache_result(n, r, nullptr);
        }
        else {
            expr_ref old(m_manager);
            proof * p;
            old = mk_p_arg(n);
            if (old == r)
                p = nullptr;
            else
                p = m_manager.mk_rewrite(old, r);
            cache_result(n, r, p);
        }
    }
}

void pull_ite_tree::operator()(app * n, app_ref & r, proof_ref & pr) {
    unsigned num_args = n->get_num_args();
    m_args.resize(num_args);
    m_p = n->get_decl();
    expr * ite = nullptr;
    for (unsigned i = 0; i < num_args; i++) {
        expr * arg = n->get_arg(i);
        if (ite) {
            m_args[i] = arg;
        }
        else if (m_manager.is_ite(arg)) {
            m_arg_idx = i;
            m_args[i] = 0;
            ite       = arg;
        }
        else {
            m_args[i] = arg;
        }
    }
    if (!ite) {
        r = n;
        pr = nullptr;
        return;
    }
    m_todo.push_back(ite);
    while (!m_todo.empty()) {
        expr * n = m_todo.back();
        if (is_cached(n))
            m_todo.pop_back();
        else if (visit_children(n)) {
            m_todo.pop_back();
            reduce(n);
        }
    }
    SASSERT(is_cached(ite));
    expr *   _r = nullptr;
    proof * _pr = nullptr;
    get_cached(ite, _r, _pr);
    r  = to_app(_r);
    pr = _pr;
    m_cache.reset();
    m_todo.reset();
}



pull_ite_tree_cfg::pull_ite_tree_cfg(ast_manager & m):
    m(m),
    m_trail(m),
    m_proc(m) {
}

bool pull_ite_tree_cfg::get_subst(expr * n, expr* & r, proof* & p) {
    if (is_app(n) && is_target(to_app(n))) {
        app_ref tmp(m);
        proof_ref pr(m);
        m_proc(to_app(n), tmp, pr);
        if (tmp != n) {
            r = tmp;
            p = pr;
            m_trail.push_back(r);
            m_trail.push_back(p);
            return true;
        }
    }
    return false;
}

bool pull_cheap_ite_tree_cfg::is_target(app * n) const {
    bool r = 
        n->get_num_args() == 2 &&
        n->get_family_id() != null_family_id &&
        m.is_bool(n) &&
        (m.is_value(n->get_arg(0)) || m.is_value(n->get_arg(1))) &&
        (m.is_term_ite(n->get_arg(0)) || m.is_term_ite(n->get_arg(1)));
    TRACE("pull_ite_target", tout << mk_pp(n, m) << "\nresult: " << r << "\n";);
    return r;
}






