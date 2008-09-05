/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      This software is part of the graphviz package      *
*                http://www.graphviz.org/                 *
*                                                         *
*            Copyright (c) 1994-2004 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
**********************************************************/

/* FIXME - incomplete replacement for codegen */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "const.h"

#include "gvplugin_render.h"
#include "gvplugin_device.h"
#include "gvcint.h"
#include "graph.h"
#include "types.h"		/* need the SVG font name schemes */

typedef enum { FORMAT_TK, } format_type;

static char *tkgen_string(char *s)
{
    return s;
}

static void tkgen_print_color(GVJ_t * job, gvcolor_t color)
{
    switch (color.type) {
    case COLOR_STRING:
	gvdevice_fputs(job, color.u.string);
	break;
    case RGBA_BYTE:
	if (color.u.rgba[3] == 0) /* transparent */
	    gvdevice_fputs(job, "\"\"");
	else
	    gvdevice_printf(job, "#%02x%02x%02x",
		color.u.rgba[0], color.u.rgba[1], color.u.rgba[2]);
	break;
    default:
	assert(0);		/* internal error */
    }
}

static void tkgen_print_tags(GVJ_t *job)
{
    char *ObjType;
    unsigned int ObjId;
    obj_state_t *obj = job->obj;

    switch (obj->emit_state) {
    case EMIT_NDRAW:
	ObjType = "node";
        ObjId = obj->u.n->id;
	break;
    case EMIT_NLABEL:
	ObjType = "node label";
        ObjId = obj->u.n->id;
	break;
    case EMIT_EDRAW:
    case EMIT_TDRAW:
    case EMIT_HDRAW:
	ObjType = "edge";
        ObjId = obj->u.e->id;
	break;
    case EMIT_ELABEL:
    case EMIT_TLABEL:
    case EMIT_HLABEL:
	ObjType = "edge label";
        ObjId = obj->u.e->id;
	break;
    case EMIT_GDRAW:
	ObjType = "graph";
	ObjId = -1;  /* hack! */
	break;
    case EMIT_GLABEL:
	ObjType = "graph label";
	ObjId = -1;  /* hack! */
	break;
    case EMIT_CDRAW:
	ObjType = "cluster";
	ObjId = obj->u.sg->meta_node->id;
	break;
    case EMIT_CLABEL:
	ObjType = "cluster label";
	ObjId = obj->u.sg->meta_node->id;
	break;
    default:
	assert (0);
	break;
    }
    gvdevice_printf(job, " -tags {id%ld %s}", ObjId, ObjType);
}

static void tkgen_canvas(GVJ_t * job)
{
   if (job->external_context) 
	gvdevice_fputs(job, job->imagedata);
   else
	gvdevice_fputs(job, "$c");
}

static void tkgen_comment(GVJ_t * job, char *str)
{
    gvdevice_fputs(job, "# ");
    gvdevice_fputs(job, tkgen_string(str));
    gvdevice_fputs(job, "\n");
}

static void tkgen_begin_job(GVJ_t * job)
{
    gvdevice_fputs(job, "# Generated by ");
    gvdevice_fputs(job, tkgen_string(job->common->info[0]));
    gvdevice_fputs(job, " version ");
    gvdevice_fputs(job, tkgen_string(job->common->info[1]));
    gvdevice_fputs(job, " (");
    gvdevice_fputs(job, tkgen_string(job->common->info[2]));
    gvdevice_fputs(job, ")\n#     For user: ");
    gvdevice_fputs(job, tkgen_string(job->common->user));
    gvdevice_fputs(job, "\n");
}

static void tkgen_begin_graph(GVJ_t * job)
{
    obj_state_t *obj = job->obj;

    gvdevice_fputs(job, "#");
    if (obj->u.g->name[0]) {
        gvdevice_fputs(job, " Title: ");
	gvdevice_fputs(job, tkgen_string(obj->u.g->name));
    }
    gvdevice_printf(job, " Pages: %d\n", job->pagesArraySize.x * job->pagesArraySize.y);
}

static void tkgen_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
    obj_state_t *obj = job->obj;

    if (obj->pen != PEN_NONE) {
        tkgen_canvas(job);
        gvdevice_fputs(job, " create text ");
        p.y -= para->fontsize * 0.5; /* cl correction */
        gvdevice_printpointf(job, p);
        gvdevice_fputs(job, " -text {");
        gvdevice_fputs(job, para->str);
        gvdevice_fputs(job, "}");
        gvdevice_fputs(job, " -fill ");
        tkgen_print_color(job, obj->pencolor);
        gvdevice_fputs(job, " -font {");
        gvdevice_fputs(job, para->fontname);
        gvdevice_printf(job, " %g}", para->fontsize);
        switch (para->just) {
        case 'l':
            gvdevice_fputs(job, " -anchor w");
            break;
        case 'r':
            gvdevice_fputs(job, " -anchor e");
            break;
        default:
        case 'n':
            break;
        }
        gvdevice_fputs(job, " -state disabled");
        tkgen_print_tags(job);
        gvdevice_fputs(job, "\n");
    }
}

static void tkgen_ellipse(GVJ_t * job, pointf * A, int filled)
{
    obj_state_t *obj = job->obj;
    pointf r;

    if (obj->pen != PEN_NONE) {
    /* A[] contains 2 points: the center and top right corner. */
        r.x = A[1].x - A[0].x;
        r.y = A[1].y - A[0].y;
        A[0].x -= r.x;
        A[0].y -= r.y;
        tkgen_canvas(job);
        gvdevice_fputs(job, " create oval ");
        gvdevice_printpointflist(job, A, 2);
        gvdevice_fputs(job, " -fill ");
        if (filled)
            tkgen_print_color(job, obj->fillcolor);
        else  /* tk ovals default to no fill, some fill
                 is necessary else "canvas find overlapping" doesn't
                 work as expected, use white instead */
	    gvdevice_fputs(job, "white");
        gvdevice_fputs(job, " -width ");
        gvdevice_printnum(job, obj->penwidth);
        gvdevice_fputs(job, " -outline ");
	tkgen_print_color(job, obj->pencolor);
        if (obj->pen == PEN_DASHED)
	    gvdevice_fputs(job, " -dash 5");
        if (obj->pen == PEN_DOTTED)
	    gvdevice_fputs(job, " -dash 2");
        tkgen_print_tags(job);
        gvdevice_fputs(job, "\n");
    }
}

static void
tkgen_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start,
	      int arrow_at_end, int filled)
{
    obj_state_t *obj = job->obj;

    if (obj->pen != PEN_NONE) {
        tkgen_canvas(job);
        gvdevice_fputs(job, " create line ");
        gvdevice_printpointflist(job, A, n);
        gvdevice_fputs(job, " -fill ");
        tkgen_print_color(job, obj->pencolor);
        gvdevice_fputs(job, " -width ");
        gvdevice_printnum(job, obj->penwidth);
        if (obj->pen == PEN_DASHED)
	    gvdevice_fputs(job, " -dash 5");
        if (obj->pen == PEN_DOTTED)
	    gvdevice_fputs(job, " -dash 2");
        gvdevice_fputs(job, " -smooth bezier");
        gvdevice_fputs(job, " -state disabled");
        tkgen_print_tags(job);
        gvdevice_fputs(job, "\n");
    }
}

static void tkgen_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    obj_state_t *obj = job->obj;

    if (obj->pen != PEN_NONE) {
        tkgen_canvas(job);
        gvdevice_fputs(job, " create polygon ");
        gvdevice_printpointflist(job, A, n);
        if (filled) {
            gvdevice_fputs(job, " -fill ");
	    tkgen_print_color(job, obj->fillcolor);
        }
        gvdevice_fputs(job, " -width ");
        gvdevice_printnum(job, obj->penwidth);
        gvdevice_fputs(job, " -outline ");
	tkgen_print_color(job, obj->pencolor);
        if (obj->pen == PEN_DASHED)
	    gvdevice_fputs(job, " -dash 5");
        if (obj->pen == PEN_DOTTED)
	    gvdevice_fputs(job, " -dash 2");
        gvdevice_fputs(job, " -state disabled");
        tkgen_print_tags(job);
        gvdevice_fputs(job, "\n");
    }
}

static void tkgen_polyline(GVJ_t * job, pointf * A, int n)
{
    obj_state_t *obj = job->obj;

    if (obj->pen != PEN_NONE) {
        tkgen_canvas(job);
        gvdevice_fputs(job, " create line ");
        gvdevice_printpointflist(job, A, n);
        gvdevice_fputs(job, " -fill ");
        tkgen_print_color(job, obj->pencolor);
        if (obj->pen == PEN_DASHED)
	    gvdevice_fputs(job, " -dash 5");
        if (obj->pen == PEN_DOTTED)
	    gvdevice_fputs(job, " -dash 2");
        gvdevice_fputs(job, " -state disabled");
        tkgen_print_tags(job);
        gvdevice_fputs(job, "\n");
    }
}

gvrender_engine_t tkgen_engine = {
    tkgen_begin_job,
    0,				/* tkgen_end_job */
    tkgen_begin_graph,
    0,				/* tkgen_end_graph */
    0, 				/* tkgen_begin_layer */
    0, 				/* tkgen_end_layer */
    0, 				/* tkgen_begin_page */
    0, 				/* tkgen_end_page */
    0, 				/* tkgen_begin_cluster */
    0, 				/* tkgen_end_cluster */
    0,				/* tkgen_begin_nodes */
    0,				/* tkgen_end_nodes */
    0,				/* tkgen_begin_edges */
    0,				/* tkgen_end_edges */
    0,				/* tkgen_begin_node */
    0,				/* tkgen_end_node */
    0,				/* tkgen_begin_edge */
    0,				/* tkgen_end_edge */
    0,				/* tkgen_begin_anchor */
    0,				/* tkgen_end_anchor */
    tkgen_textpara,
    0,				/* tkgen_resolve_color */
    tkgen_ellipse,
    tkgen_polygon,
    tkgen_bezier,
    tkgen_polyline,
    tkgen_comment,
    0,				/* tkgen_library_shape */
};

gvrender_features_t render_features_tk = {
    GVRENDER_Y_GOES_DOWN
	| GVRENDER_NO_BG, /* flags */
    4.,                         /* default pad - graph units */
    NULL, 			/* knowncolors */
    0,				/* sizeof knowncolors */
    COLOR_STRING,		/* color_type */
};

gvdevice_features_t device_features_tk = {
    0,				/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

gvplugin_installed_t gvrender_tk_types[] = {
    {FORMAT_TK, "tk", 1, &tkgen_engine, &render_features_tk},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_tk_types[] = {
    {FORMAT_TK, "tk:tk", 1, NULL, &device_features_tk},
    {0, NULL, 0, NULL, NULL}
};
