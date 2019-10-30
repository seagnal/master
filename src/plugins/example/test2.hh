/***********************************************************************
 ** test2.hh
 ***********************************************************************
 ** Copyright (c) SEAGNAL SAS
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms are permitted
 ** provided that the above copyright notice and this paragraph are
 ** duplicated in all such forms and that any documentation,
 ** advertising materials, and other materials related to such
 ** distribution and use acknowledge that the software was developed
 ** by the SEAGNAL. The name of the
 ** SEAGNAL may not be used to endorse or promote products derived
 ** from this software without specific prior written permission.
 ** THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 ** IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 ** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ***********************************************************************/

/* define against mutual inclusion */
#ifndef TEST2_HH_
#define TEST2_HH_

/**
 * @file test2.hh
 * TEST2 Plugin definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2016
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <c/common.h>
#include <plugin.hh>
#include <host.hh>
/***********************************************************************
 * Defines
 ***********************************************************************/
enum ET_TEST_PORTS {
	E_TEST_PORT_1,
	E_TEST_PORT_2,
	E_TEST_PORT_3,
};
/***********************************************************************
 * Types
 ***********************************************************************/


class CT_TEST2: public CT_CORE {
public:
	CT_TEST2();
	virtual ~CT_TEST2();

	/* Port handler function */
	int f_event(CT_GUARD<CT_PORT_NODE> const & in_pc_node);

	/* Virtual plugin - events - Settings */
	int f_settings(CT_NODE & in_c_node);
	/* Virtual plugin - events - Post initialization */
	int f_post_init(void);
	/* Virtual plugin - events - Pre destroy of application */
	int f_pre_destroy(void);
	/* Virtual plugin - events - Post profile */
	int f_post_profile(void);
	/* Virtual plugin - events - Pre profile */
	int f_pre_profile(void);
};

#endif /* TEST2_HH_ */
