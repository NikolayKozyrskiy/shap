#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <numpy/arrayobject.h>
#include "tree_shap.h"
#include <iostream>

typedef double tfloat;

// Have a treeshapdependent and a treeshapindependent
static PyObject *_cext_tree_shap(PyObject *self, PyObject *args);
static PyObject *_cext_compute_expectations(PyObject *self, PyObject *args);
static PyObject *_cext_tree_shap_indep(PyObject *self, PyObject *args);

static PyMethodDef module_methods[] = {
  {"tree_shap", _cext_tree_shap, METH_VARARGS, "C implementation of Tree SHAP."},
  {"compute_expectations", _cext_compute_expectations, METH_VARARGS, "Compute expectations of internal nodes."},
  {"tree_shap_indep", _cext_tree_shap_indep, METH_VARARGS, "C implementation of Independent Tree SHAP."},
  {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "_cext",
  "This module provides an interface for a fast Tree SHAP implementation.",
  -1,
  module_methods,
  NULL,
  NULL,
  NULL,
  NULL
};
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit__cext(void)
#else
PyMODINIT_FUNC init_cext(void)
#endif
{
  #if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
    if (!module) return NULL;
  #else
    PyObject *module = Py_InitModule("_cext", module_methods);
    if (!module) return;
  #endif

  /* Load `numpy` functionality. */
  import_array();

  #if PY_MAJOR_VERSION >= 3
    return module;
  #endif
}

static PyObject *_cext_compute_expectations(PyObject *self, PyObject *args)
{
  PyObject *children_left_obj;
  PyObject *children_right_obj;
  PyObject *node_sample_weight_obj;
  PyObject *values_obj;
    
  /* Parse the input tuple */
  if (!PyArg_ParseTuple(
    args, "OOOO", &children_left_obj, &children_right_obj, &node_sample_weight_obj, &values_obj
  )) return NULL;

  /* Interpret the input objects as numpy arrays. */
  PyArrayObject *children_left_array = (PyArrayObject*)PyArray_FROM_OTF(children_left_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *children_right_array = (PyArrayObject*)PyArray_FROM_OTF(children_right_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *node_sample_weight_array = (PyArrayObject*)PyArray_FROM_OTF(node_sample_weight_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *values_array = (PyArrayObject*)PyArray_FROM_OTF(values_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);

  /* If that didn't work, throw an exception. */
  if (children_left_array == NULL || children_right_array == NULL ||
      values_array == NULL || node_sample_weight_array == NULL) {
    Py_XDECREF(children_left_array);
    Py_XDECREF(children_right_array);
    Py_XDECREF(values_array);
    Py_XDECREF(node_sample_weight_array);
    return NULL;
  }

  // number of outputs
  const unsigned num_outputs = PyArray_DIM(values_array, 1);

  /* Get pointers to the data as C-types. */
  int *children_left = (int*)PyArray_DATA(children_left_array);
  int *children_right = (int*)PyArray_DATA(children_right_array);
  tfloat *values = (tfloat*)PyArray_DATA(values_array);
  tfloat *node_sample_weight = (tfloat*)PyArray_DATA(node_sample_weight_array);

  const int max_depth = compute_expectations(
    num_outputs, children_left, children_right, node_sample_weight, values, 0, 0
  );

  // clean up the created python objects
  Py_XDECREF(children_left_array);
  Py_XDECREF(children_right_array);
  Py_XDECREF(values_array);
  Py_XDECREF(node_sample_weight_array);

  PyObject *ret = Py_BuildValue("i", max_depth);
  return ret;
}

static PyObject *_cext_tree_shap(PyObject *self, PyObject *args)
{
  int max_depth;
  PyObject *children_left_obj;
  PyObject *children_right_obj;
  PyObject *children_default_obj;
  PyObject *features_obj;
  PyObject *thresholds_obj;
  PyObject *values_obj;
  PyObject *node_sample_weight_obj;
  PyObject *x_obj;
  PyObject *x_missing_obj;
  PyObject *out_contribs_obj;
  int condition;
  int condition_feature;
  bool less_than_or_equal;

  /* Parse the input tuple */
  if (!PyArg_ParseTuple(
    args, "iOOOOOOOOOOiib", &max_depth, &children_left_obj, &children_right_obj, &children_default_obj,
    &features_obj, &thresholds_obj, &values_obj, &node_sample_weight_obj, &x_obj,
    &x_missing_obj, &out_contribs_obj, &condition, &condition_feature, &less_than_or_equal
  )) return NULL;

  /* Interpret the input objects as numpy arrays. */
  PyArrayObject *children_left_array = (PyArrayObject*)PyArray_FROM_OTF(children_left_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *children_right_array = (PyArrayObject*)PyArray_FROM_OTF(children_right_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *children_default_array = (PyArrayObject*)PyArray_FROM_OTF(children_default_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *features_array = (PyArrayObject*)PyArray_FROM_OTF(features_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *thresholds_array = (PyArrayObject*)PyArray_FROM_OTF(thresholds_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *values_array = (PyArrayObject*)PyArray_FROM_OTF(values_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *node_sample_weight_array = (PyArrayObject*)PyArray_FROM_OTF(node_sample_weight_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *x_array = (PyArrayObject*)PyArray_FROM_OTF(x_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *x_missing_array = (PyArrayObject*)PyArray_FROM_OTF(x_missing_obj, NPY_BOOL, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *out_contribs_array = (PyArrayObject*)PyArray_FROM_OTF(out_contribs_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);

  /* If that didn't work, throw an exception. */
  if (children_left_array == NULL || children_right_array == NULL ||
      children_default_array == NULL || features_array == NULL || thresholds_array == NULL ||
      values_array == NULL || node_sample_weight_array == NULL || x_array == NULL ||
      x_missing_array == NULL || out_contribs_array == NULL) {
    Py_XDECREF(children_left_array);
    Py_XDECREF(children_right_array);
    Py_XDECREF(children_default_array);
    Py_XDECREF(features_array);
    Py_XDECREF(thresholds_array);
    Py_XDECREF(values_array);
    Py_XDECREF(node_sample_weight_array);
    Py_XDECREF(x_array);
    Py_XDECREF(x_missing_array);
    Py_XDECREF(out_contribs_array);
    return NULL;
  }

  // number of features
  const unsigned M = PyArray_DIM(x_array, 0);

  // number of outputs
  const unsigned num_outputs = PyArray_DIM(values_array, 1);

  // Get pointers to the data as C-types
  int *children_left = (int*)PyArray_DATA(children_left_array);
  int *children_right = (int*)PyArray_DATA(children_right_array);
  int *children_default = (int*)PyArray_DATA(children_default_array);
  int *features = (int*)PyArray_DATA(features_array);
  tfloat *thresholds = (tfloat*)PyArray_DATA(thresholds_array);
  tfloat *values = (tfloat*)PyArray_DATA(values_array);
  tfloat *node_sample_weight = (tfloat*)PyArray_DATA(node_sample_weight_array);
  tfloat *x = (tfloat*)PyArray_DATA(x_array);
  bool *x_missing = (bool*)PyArray_DATA(x_missing_array);
  tfloat *out_contribs = (tfloat*)PyArray_DATA(out_contribs_array);

  //const int max_depth = compute_expectations(children_left, children_right, node_sample_weight, values, 0, 0);

  tree_shap(
    M, num_outputs, max_depth, children_left, children_right, children_default, features,
    thresholds, values, node_sample_weight, x, x_missing, out_contribs,
    condition, condition_feature, less_than_or_equal
  );

  // clean up the created python objects
  Py_XDECREF(children_left_array);
  Py_XDECREF(children_right_array);
  Py_XDECREF(children_default_array);
  Py_XDECREF(features_array);
  Py_XDECREF(thresholds_array);
  Py_XDECREF(values_array);
  Py_XDECREF(node_sample_weight_array);
  Py_XDECREF(x_array);
  Py_XDECREF(x_missing_array);
  Py_XDECREF(out_contribs_array);

  /* Build the output tuple */
  PyObject *ret = Py_BuildValue("d", (double)values[0]);
  return ret;
}

static PyObject *_cext_tree_shap_indep(PyObject *self, PyObject *args)
{
  int max_depth;
  PyObject *children_left_obj;
  PyObject *children_right_obj;
  PyObject *children_default_obj;
  PyObject *features_obj;
  PyObject *thresholds_obj;
  PyObject *values_obj;
  PyObject *x_obj;
  PyObject *x_missing_obj;
  PyObject *r_obj;
  PyObject *r_missing_obj;
  PyObject *out_contribs_obj;

  /* Parse the input tuple */
  if (!PyArg_ParseTuple(
    args, "iOOOOOOOOOOO", &max_depth, &children_left_obj, &children_right_obj,
    &children_default_obj, &features_obj, &thresholds_obj, &values_obj, 
    &x_obj, &x_missing_obj, &r_obj, &r_missing_obj, &out_contribs_obj
  )) return NULL;

  /* Interpret the input objects as numpy arrays. */
  PyArrayObject *children_left_array = (PyArrayObject*)PyArray_FROM_OTF(children_left_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *children_right_array = (PyArrayObject*)PyArray_FROM_OTF(children_right_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *children_default_array = (PyArrayObject*)PyArray_FROM_OTF(children_default_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *features_array = (PyArrayObject*)PyArray_FROM_OTF(features_obj, NPY_INT, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *thresholds_array = (PyArrayObject*)PyArray_FROM_OTF(thresholds_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *values_array = (PyArrayObject*)PyArray_FROM_OTF(values_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *x_array = (PyArrayObject*)PyArray_FROM_OTF(x_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *x_missing_array = (PyArrayObject*)PyArray_FROM_OTF(x_missing_obj, NPY_BOOL, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *r_array = (PyArrayObject*)PyArray_FROM_OTF(r_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *r_missing_array = (PyArrayObject*)PyArray_FROM_OTF(r_missing_obj, NPY_BOOL, NPY_ARRAY_IN_ARRAY);
  PyArrayObject *out_contribs_array = (PyArrayObject*)PyArray_FROM_OTF(out_contribs_obj, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);

  /* If that didn't work, throw an exception. */
  if (children_left_array == NULL || children_right_array == NULL || 
      children_default_array == NULL || features_array == NULL || 
      thresholds_array == NULL || values_array == NULL || x_array == NULL || 
      x_missing_array == NULL || r_array == NULL || r_missing_array == NULL || 
      out_contribs_array == NULL) {
    Py_XDECREF(children_left_array);
    Py_XDECREF(children_right_array);
    Py_XDECREF(children_default_array);
    Py_XDECREF(features_array);
    Py_XDECREF(thresholds_array);
    Py_XDECREF(values_array);
    Py_XDECREF(x_array);
    Py_XDECREF(x_missing_array);
    Py_XDECREF(r_array);
    Py_XDECREF(r_missing_array);
    Py_XDECREF(out_contribs_array);
    return NULL;
  }

  // number of features
  const unsigned num_feats = PyArray_DIM(x_array, 0);

  // number of nodes
  const unsigned num_nodes = PyArray_DIM(features_array, 0);

  // Get pointers to the data as C-types
  int *children_left = (int*)PyArray_DATA(children_left_array);
  int *children_right = (int*)PyArray_DATA(children_right_array);
  int *children_default = (int*)PyArray_DATA(children_default_array);
  int *features = (int*)PyArray_DATA(features_array);
  tfloat *thresholds = (tfloat*)PyArray_DATA(thresholds_array);
  tfloat *values = (tfloat*)PyArray_DATA(values_array);
  tfloat *x = (tfloat*)PyArray_DATA(x_array);
  bool *x_missing = (bool*)PyArray_DATA(x_missing_array);
  tfloat *r = (tfloat*)PyArray_DATA(r_array);
  bool *r_missing = (bool*)PyArray_DATA(r_missing_array);
  tfloat *out_contribs = (tfloat*)PyArray_DATA(out_contribs_array);
    
  // Preallocating things    
  Node *mytree = new Node[num_nodes];
  for (unsigned i = 0; i < num_nodes; ++i) {
    mytree[i].cl = children_left[i];
    mytree[i].cr = children_right[i];
    mytree[i].cd = children_default[i];
    if (i == 0) {
      mytree[i].pnode = 0;
    }
    if (children_left[i] >= 0) {
      mytree[children_left[i]].pnode = i;
      mytree[children_left[i]].pfeat = features[i];
    }
    if (children_right[i] >= 0) {
      mytree[children_right[i]].pnode = i;
      mytree[children_right[i]].pfeat = features[i];
    }

    mytree[i].thres = thresholds[i];
    mytree[i].value = values[i];
    mytree[i].feat = features[i];
  }
    
  float *pos_lst = new float[num_nodes];
  float *neg_lst = new float[num_nodes];
  int *node_stack = new int[(unsigned) max_depth];
  signed short *feat_hist = new signed short[num_feats];
  float *memoized_weights = new float[30*30];
  for (int n = 0; n < 30; ++n) {
    for (int m = 0; m < 30; ++m) {
      memoized_weights[n+30*m] = calc_weight(n, m);
    }
  }

  tree_shap_indep(
      max_depth, num_feats, num_nodes, x, x_missing, r, r_missing, 
      out_contribs, pos_lst, neg_lst, feat_hist, memoized_weights, 
      node_stack, mytree
  );
  delete[] mytree;
  delete[] pos_lst;
  delete[] neg_lst;
  delete[] node_stack;
  delete[] feat_hist;
  delete[] memoized_weights;

  // clean up the created python objects
  Py_XDECREF(children_left_array);
  Py_XDECREF(children_right_array);
  Py_XDECREF(children_default_array);
  Py_XDECREF(features_array);
  Py_XDECREF(thresholds_array);
  Py_XDECREF(values_array);
  Py_XDECREF(x_array);
  Py_XDECREF(x_missing_array);
  Py_XDECREF(r_array);
  Py_XDECREF(r_missing_array);
  Py_XDECREF(out_contribs_array);

  /* Build the output tuple */
  PyObject *ret = Py_BuildValue("d", (double)values[0]);
  return ret;
}
