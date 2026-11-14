/*
 * Copyright 2012 Intel Corporation
 * Copyright 2015,2019 Collabora, Ltd.
 * Copyright 2016 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
static const char vertex_shader[] =
    "/*\n"
    " * Copyright 2012 Intel Corporation\n"
    " * Copyright 2015,2019 Collabora, Ltd.\n"
    " * Copyright 2016 NVIDIA Corporation\n"
    " *\n"
    " * Permission is hereby granted, free of charge, to any person obtaining\n"
    " * a copy of this software and associated documentation files (the\n"
    " * \"Software\"), to deal in the Software without restriction, including\n"
    " * without limitation the rights to use, copy, modify, merge, publish,\n"
    " * distribute, sublicense, and/or sell copies of the Software, and to\n"
    " * permit persons to whom the Software is furnished to do so, subject to\n"
    " * the following conditions:\n"
    " *\n"
    " * The above copyright notice and this permission notice (including the\n"
    " * next paragraph) shall be included in all copies or substantial\n"
    " * portions of the Software.\n"
    " *\n"
    " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
    " * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
    " * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
    " * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS\n"
    " * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN\n"
    " * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
    " * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
    " * SOFTWARE.\n"
    " */\n"
    "\n"
    "/* For annotating shader compile-time constant arguments */\n"
    "#define compile_const const\n"
    "\n"
    "/* enum gl_shader_texcoord_input */\n"
    "#define SHADER_TEXCOORD_INPUT_ATTRIB  0\n"
    "#define SHADER_TEXCOORD_INPUT_SURFACE 1\n"
    "\n"
    "/* Always use high-precision for vertex calculations */\n"
    "precision highp float;\n"
    "\n"
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "#define FRAG_PRECISION highp\n"
    "#else\n"
    "#define FRAG_PRECISION mediump\n"
    "#endif\n"
    "\n"
    "uniform mat4 proj;\n"
    "uniform mat4 surface_to_buffer;\n"
    "\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord;\n"
    "attribute vec4 barycentric;\n"
    "\n"
    "/* Match the varying precision to the fragment shader */\n"
    "varying FRAG_PRECISION vec2 v_texcoord;\n"
    "varying FRAG_PRECISION vec3 v_barycentric;\n"
    "\n"
    "compile_const int c_texcoord_input = DEF_TEXCOORD_INPUT;\n"
    "compile_const bool c_wireframe = DEF_WIREFRAME;\n"
    "\n"
    "void main()\n"
    "{\n"
    "\tgl_Position = proj * vec4(position, 0.0, 1.0);\n"
    "\n"
    "\tif (c_texcoord_input == SHADER_TEXCOORD_INPUT_ATTRIB)\n"
    "\t\tv_texcoord = texcoord;\n"
    "\telse if (c_texcoord_input == SHADER_TEXCOORD_INPUT_SURFACE)\n"
    "\t\tv_texcoord = vec2(surface_to_buffer * vec4(position, 0.0, 1.0));\n"
    "\n"
    "\tif (c_wireframe)\n"
    "\t\tv_barycentric = barycentric.xyz;\n"
    "}\n";
