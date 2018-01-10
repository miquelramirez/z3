/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    numeral_factory.h

Abstract:

    <abstract>

Author:

    Leonardo de Moura (leonardo) 2008-10-28.

Revision History:

--*/
#ifndef NUMERAL_FACTORY_H_
#define NUMERAL_FACTORY_H_

#include "smt/proto_model/value_factory.h"
#include "ast/arith_decl_plugin.h"
#include "ast/bv_decl_plugin.h"

class numeral_factory : public simple_factory<rational> {
public:
    numeral_factory(ast_manager & m, family_id fid):simple_factory<rational>(m, fid) {}
    virtual ~numeral_factory() {}
};    

class arith_factory : public numeral_factory {
    arith_util     m_util;

    virtual app * mk_value_core(rational const & val, sort * s);

public:
    arith_factory(ast_manager & m);
    virtual ~arith_factory();

    app * mk_num_value(rational const & val, bool is_int);
};

class bv_factory : public numeral_factory {
    bv_util         m_util;

    virtual app * mk_value_core(rational const & val, sort * s);

public:
    bv_factory(ast_manager & m);
    virtual ~bv_factory();

    app * mk_num_value(rational const & val, unsigned bv_size);
};

#endif /* NUMERAL_FACTORY_H_ */

