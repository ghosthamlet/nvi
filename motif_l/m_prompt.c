/*-
 * Copyright (c) 1996
 *	Rob Zimmermann.  All rights reserved.
 * Copyright (c) 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "$Id: m_prompt.c,v 8.1 1996/12/10 19:16:37 bostic Exp $ (Berkeley) $Date: 1996/12/10 19:16:37 $";
#endif /* not lint */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <Xm/MessageB.h>


void	vi_fatal_message( parent, str )
Widget	parent;
String	str;
{
    Widget	db = XmCreateErrorDialog( parent, "Fatal", NULL, 0 );
    XmString	msg = XmStringCreateSimple( str );
    extern	void _vi_cancel_cb();

    XtVaSetValues( db,
		   XmNmessageString,	msg,
		   0
		   );
    XtAddCallback( XtParent(db), XmNpopdownCallback, _vi_cancel_cb, 0 );

    XtUnmanageChild( XmMessageBoxGetChild( db, XmDIALOG_CANCEL_BUTTON ) );
    XtUnmanageChild( XmMessageBoxGetChild( db, XmDIALOG_HELP_BUTTON ) );

    _vi_modal_dialog( db );

    exit(0);
}


void	vi_info_message( parent, str )
Widget	parent;
String	str;
{
    static	Widget	db = NULL;
    XmString	msg = XmStringCreateSimple( str );
    extern	void _vi_cancel_cb();

    if ( db == NULL )
	db = XmCreateInformationDialog( parent, "Information", NULL, 0 );

    XtVaSetValues( db,
		   XmNmessageString,	msg,
		   0
		   );
    XtAddCallback( XtParent(db), XmNpopdownCallback, _vi_cancel_cb, 0 );

    XtUnmanageChild( XmMessageBoxGetChild( db, XmDIALOG_CANCEL_BUTTON ) );
    XtUnmanageChild( XmMessageBoxGetChild( db, XmDIALOG_HELP_BUTTON ) );

    _vi_modal_dialog( db );
}
