#pragma once

#include <Evas.h>

// Tizen 6.0's libevas exports the legacy VG path API but its headers comment it
// out; 6.5 publishes the same prototypes (efl-rendering.md, section 8).
#if RN_TIZEN_DECLARE_EVAS_VG
extern "C" {
EAPI void evas_object_vg_root_node_set(Evas_Object *obj, Evas_Vg_Node *root);
EAPI Evas_Vg_Gradient_Linear *evas_vg_gradient_linear_add(Evas_Vg_Container *parent);
EAPI Evas_Vg_Gradient_Radial *evas_vg_gradient_radial_add(Evas_Vg_Container *parent);
EAPI void evas_vg_shape_append_close(Evas_Vg_Shape *obj);
EAPI void evas_vg_shape_append_cubic_to(
    Evas_Vg_Shape *obj, double x, double y, double ctrl_x0, double ctrl_y0, double ctrl_x1, double ctrl_y1);
EAPI void evas_vg_shape_append_line_to(Evas_Vg_Shape *obj, double x, double y);
EAPI void evas_vg_shape_append_move_to(Evas_Vg_Shape *obj, double x, double y);
EAPI void evas_vg_shape_append_rect(Evas_Vg_Shape *obj, double x, double y, double w, double h, double rx, double ry);
EAPI void evas_vg_shape_reset(Evas_Vg_Shape *obj);
}
#endif
