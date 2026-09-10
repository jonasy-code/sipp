/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright Equivalent Software HB  1992
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 1, or any later version.
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** You can receive a copy of the GNU General Public License from the
 ** Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

/**
 ** objects.c - Functions that handles object and surface creation
 **             and the object hierarchies and the object database.
 **/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <objects.h>
#include <sipp.h>
#include <smalloc.h>

Object *sipp_world; /* The world that is rendered */

static Vertex *vertex_list;       /* Vertices of the surface being built */
static Vertex *first_vert;        /* First vertex pushed in this surface */
static Vertex_ref *vertex_stack;  /* Vertex stack for current polygon. */
static int nvertices;             /* Number of vertices on the stack */
static Vertex_ref *vstack_bottom; /* Last entry in vertex stack. */
static Polygon *poly_stack;       /* Polygon stack for current object. */
static int first_vertex;          /* Used when determining if we are   */
                                  /* installing the first vertex in an */
                                  /* object. *Not* a boolean!          */
static double dist_limit;         /* Minimal distance between two      */
                                  /* vertices without them being       */
                                  /* considered to be the same vertex. */
static bool surface_desc_hdrs;    /* When set, surfaces that are    */
                                  /* defined have headers.          */
#ifdef FFD
static bool ffd_desc_hdrs; /* When set, ffd_data:s that are     */
                           /* defined have headers.             */
#endif
static bool user_refcount; /* True if users pointer to objects  */
                           /* and surfaces should be counted    */

/*
 * Initial and increment sizes of the surface and sub-object arrays stored
 * in an object.
 */
enum {
  SURFACES_INIT_SIZE = 32,
  SURFACES_INCR_SIZE = 12,
  SUB_OBJS_INIT_SIZE = 12,
  SUB_OBJS_INCR_SIZE = 12
};

/*
 * Prototypes of internal functions.
 */
static Vertex *vertex_lookup(Vector *pos, Vector *texture, Vector *normal,
                             bool use_norm);

static void vhash_rebuild(unsigned new_size, double new_cell_size);

static void push_vertex(double x, double y, double z, double u, double v,
                        double w, double nx, double ny, double nz,
                        bool use_norm);

static Vertex *copy_vertices(Vertex *vp);

static Polygon *copy_polygons(Polygon *pp, Surface *surface);

static Surface *surface_copy(Surface *surface);

static void delete_vertices(Vertex **vtree);

static Object *object_copy(Object *object, bool copy_surfaces,
                           bool copy_sub_objs);

/*
 * Spatial hash table used to find already existing vertices while a
 * surface is being built.
 *
 * Space is divided into cubic cells of side cell_size, and every vertex
 * is entered into the bucket of the cell containing it.  Two vertices are
 * considered equal if all their components differ by at most dist_limit,
 * so a vertex equal to the one we are looking for can only be in the same
 * cell or, if we are within dist_limit of a cell wall, in a neighbouring
 * one.  The cells are much larger than dist_limit (VHASH_CELL_FACTOR
 * times), so almost every lookup probes a single bucket.
 *
 * dist_limit is only known once two different vertices have been pushed
 * (see push_vertex()).  cell_size follows dist_limit, and the table is
 * rehashed when it changes; that happens only while it holds a vertex or
 * two, so it is cheap.
 */
enum { VHASH_INIT_SIZE = 1024 };                /* Initial number of buckets */
static const double VHASH_CELL_FACTOR = 1024.0; /* cell_size / dist_limit */

static Vertex **vhash;       /* Bucket heads */
static unsigned vhash_size;  /* Number of buckets, a power of two */
static unsigned vhash_count; /* Number of vertices in the table */
static double cell_size;     /* Side of one hash cell */

/*
 * Integer cell coordinate (as a double, so it can never overflow) of a
 * scalar coordinate.
 */
static double cell_coord(double v) {
  double c;

  c = floor(v / cell_size);
  if (c == 0.0) {
    c = 0.0; /* Fold -0.0 into +0.0 */
  }
  return c;
}

/*
 * Hash a cell into a bucket index.
 *
 * The inputs are the IEEE bit patterns of three small integer-valued
 * doubles, so most of their bits are an identical exponent field; the
 * xor-shift/multiply tail at the end exists to fold the few varying
 * bits down into the low bits that the mask keeps.  The multipliers are
 * well-known hashing constants (any three distinct odd numbers would do
 * about as well; measured bucket occupancy on real meshes matches the
 * Poisson ideal):
 *   0x9E3779B97F4A7C15  2^64 / golden ratio, rounded to odd.  Knuth's
 *                       multiplicative hashing constant (TAOCP vol. 3,
 *                       6.4), also used in Fibonacci hashing and
 *                       boost::hash_combine.
 *   0xC2B2AE3D27D4EB4F  XXH_PRIME64_2 from Yann Collet's xxHash64.
 *   0x165667B19E3779F9  XXH_PRIME64_3 from xxHash64.
 * Correctness never depends on the hash: buckets are chained, and every
 * lookup applies the full tolerance test to each candidate.
 */
static unsigned cell_hash(double cx, double cy, double cz) {
  unsigned long long hx, hy, hz, h;

  memcpy(&hx, &cx, sizeof(hx));
  memcpy(&hy, &cy, sizeof(hy));
  memcpy(&hz, &cz, sizeof(hz));

  h = hx * 0x9E3779B97F4A7C15ULL;
  h ^= hy * 0xC2B2AE3D27D4EB4FULL;
  h ^= hz * 0x165667B19E3779F9ULL;
  h ^= h >> 32;
  h *= 0x9E3779B97F4A7C15ULL;
  h ^= h >> 29;

  return (unsigned)(h & (vhash_size - 1));
}

/*
 * Insert a vertex into the bucket for its cell.
 */
static void vhash_insert(Vertex *v) {
  unsigned h;

  h = cell_hash(cell_coord(v->pos.x), cell_coord(v->pos.y),
                cell_coord(v->pos.z));
  v->hnext = vhash[h];
  vhash[h] = v;
}

/*
 * (Re)create the bucket array with NEW_SIZE buckets and NEW_CELL_SIZE
 * as cell size, and enter all vertices in vertex_list into it.
 */
static void vhash_rebuild(unsigned new_size, double new_cell_size) {
  Vertex *v;

  if (vhash != NULL && new_size != vhash_size) {
    sfree(vhash);
    vhash = NULL;
  }
  if (vhash == NULL) {
    vhash = (Vertex **)scalloc(new_size, sizeof(Vertex *));
    vhash_size = new_size;
  } else {
    memset(vhash, 0, vhash_size * sizeof(Vertex *));
  }
  cell_size = new_cell_size;

  for (v = vertex_list; v != NULL; v = v->next) {
    vhash_insert(v);
  }
}

/*
 * Forget all vertices in the table (they now belong to a surface).
 */
static void vhash_clear(void) {
  if (vhash != NULL) {
    memset(vhash, 0, vhash_size * sizeof(Vertex *));
  }
  vhash_count = 0;
}

/*
 * Are the given coordinates "equal" to those of vertex V?
 * This is the same test as the old vertex tree performed.
 */
static bool vertex_match(Vertex *v, Vector *pos, Vector *texture,
                         Vector *normal, bool use_norm) {
  if (fabs(pos->x - v->pos.x) > dist_limit ||
      fabs(pos->y - v->pos.y) > dist_limit ||
      fabs(pos->z - v->pos.z) > dist_limit) {
    return FALSE;
  }
  if (fabs(texture->x - v->texture.x) > dist_limit ||
      fabs(texture->y - v->texture.y) > dist_limit ||
      fabs(texture->z - v->texture.z) > dist_limit) {
    return FALSE;
  }
  if (use_norm || v->fixed_normal) {
    if (fabs(normal->x - v->normal.x) > dist_limit ||
        fabs(normal->y - v->normal.y) > dist_limit ||
        fabs(normal->z - v->normal.z) > dist_limit) {
      return FALSE;
    }
  }
  return TRUE;
}

/*
 * Search for a vertex among those pushed so far in the current surface.
 * Vertices are assumed to be equal if they differ less than dist_limit
 * in all directions.
 *
 * If the vertex is not found, create it and install it in the vertex
 * list and the hash table.
 */
static Vertex *vertex_lookup(Vector *pos, Vector *texture, Vector *normal,
                             bool use_norm) {
  Vertex *v;
  double cx, cy, cz;
  double want_cell;
  int xlo, xhi, ylo, yhi, zlo, zhi;
  int dx, dy, dz;
  unsigned h;

  /*
   * Keep the cell size in step with dist_limit.  Before dist_limit
   * is known (the very first vertex ever) any positive size will do.
   */
  want_cell = ((dist_limit > 0.0) ? dist_limit : 1e-10) * VHASH_CELL_FACTOR;
  if (vhash == NULL || want_cell != cell_size) {
    vhash_rebuild((vhash == NULL) ? VHASH_INIT_SIZE : vhash_size, want_cell);
  }

  cx = cell_coord(pos->x);
  cy = cell_coord(pos->y);
  cz = cell_coord(pos->z);

  /*
   * Which neighbouring cells could contain a vertex within dist_limit
   * of pos?  Usually none.
   */
  xlo = (cell_coord(pos->x - dist_limit) < cx) ? -1 : 0;
  xhi = (cell_coord(pos->x + dist_limit) > cx) ? 1 : 0;
  ylo = (cell_coord(pos->y - dist_limit) < cy) ? -1 : 0;
  yhi = (cell_coord(pos->y + dist_limit) > cy) ? 1 : 0;
  zlo = (cell_coord(pos->z - dist_limit) < cz) ? -1 : 0;
  zhi = (cell_coord(pos->z + dist_limit) > cz) ? 1 : 0;

  for (dx = xlo; dx <= xhi; dx++) {
    for (dy = ylo; dy <= yhi; dy++) {
      for (dz = zlo; dz <= zhi; dz++) {
        h = cell_hash(cx + dx, cy + dy, cz + dz);
        for (v = vhash[h]; v != NULL; v = v->hnext) {
          if (vertex_match(v, pos, texture, normal, use_norm)) {
            return v;
          }
        }
      }
    }
  }

  /*
   * Not found: create a new vertex.
   */
  v = (Vertex *)smalloc(sizeof(Vertex));
  v->pos = *pos;
  v->texture = *texture;
  if (use_norm) {
    v->normal = *normal;
    v->fixed_normal = TRUE;
  } else {
    MakeVector(v->normal, 0.0, 0.0, 0.0);
    v->fixed_normal = FALSE;
  }
#ifdef FFD
  v->ffd_vertex = NULL;
#endif
  v->next = vertex_list;
  vertex_list = v;
  if (first_vert == NULL) {
    first_vert = v;
  }

  if (++vhash_count > vhash_size) {
    vhash_rebuild(vhash_size * 2, cell_size); /* re-inserts v too */
  } else {
    vhash_insert(v);
  }

  return v;
}

/*
 * Push a vertex on the vertex stack.
 */
static void push_vertex(double x, double y, double z, double u, double v,
                        double w, double nx, double ny, double nz,
                        bool use_norm) {
  Vector pos;
  Vector texture;
  Vector normal;
  Vertex_ref *vref;

  MakeVector(pos, x, y, z);
  MakeVector(texture, u, v, w);
  if (use_norm) {
    MakeVector(normal, nx, ny, nz);
  } else {
    MakeVector(normal, 0.0, 0.0, 0.0);
  }

  /*
   * To get a reasonable dist_limit we use the following "heuristic"
   * value:
   * The distance between the first two vertices installed in
   * the surface, multiplied by the magic number 1e-10, unless
   * they are the same vertex. In that case 1e-10 is used until
   * we get a vertex that differs from the first.
   */
  if (!first_vertex) {
    first_vertex++;
  } else if (first_vertex == 1) {
    dist_limit = sqrt((x - first_vert->pos.x) * (x - first_vert->pos.x) +
                      (y - first_vert->pos.y) * (y - first_vert->pos.y) +
                      (z - first_vert->pos.z) * (z - first_vert->pos.z)) *
                 1e-10; /* Magic!!! */
    if (dist_limit != 0.0)
      first_vertex++;
    else
      dist_limit = 1e-10; /* More Magic */
  }
  vref = (Vertex_ref *)smalloc(sizeof(Vertex_ref));
  if (vertex_stack == NULL) {
    vertex_stack = vref;
  } else {
    vstack_bottom->next = vref;
  }
  vstack_bottom = vref;
  vref->vertex = vertex_lookup(&pos, &texture, &normal, use_norm);
  vref->next = NULL;
  nvertices++;
}

/*
 * Push a vertex on the vertex stack (without texture coordinates).
 */
void vertex_push(double x, double y, double z) {
  push_vertex(x, y, z, (double)0.0, (double)0.0, (double)0.0, (double)0.0,
              (double)0.0, (double)0.0, FALSE);
}

/*
 * Push a vertex with texture coordinates on the vertex stack.
 */
void vertex_tx_push(double x, double y, double z, double u, double v,
                    double w) {
  push_vertex(x, y, z, u, v, w, (double)0.0, (double)0.0, (double)0.0, FALSE);
}

/*
 * Push a vertex with specified normal on the vertex stack.
 */
void vertex_n_push(double x, double y, double z, double nx, double ny,
                   double nz) {
  push_vertex(x, y, z, (double)0.0, (double)0.0, (double)0.0, nx, ny, nz, TRUE);
}

/*
 * Push a vertex with specified normal on the vertex stack.
 */
void vertex_tx_n_push(double x, double y, double z, double u, double v,
                      double w, double nx, double ny, double nz) {
  push_vertex(x, y, z, u, v, w, nx, ny, nz, TRUE);
}

/*
 * Push a polygon on the polygon stack. Empty the vertex stack afterwards.
 */
void polygon_push(void) {
  Polygon *polyref;
  Vertex_ref *vref;
  int i;

  if (vertex_stack != NULL) {
    polyref = (Polygon *)smalloc(sizeof(Polygon));
    polyref->vertex = (Vertex **)smalloc(nvertices * sizeof(Vertex *));
    for (i = 0; i < nvertices; i++) {
      polyref->vertex[i] = vertex_stack->vertex;
      vref = vertex_stack;
      vertex_stack = vertex_stack->next;
      sfree(vref);
    }
    polyref->nvertices = nvertices;
    nvertices = 0;
    polyref->backface = 0;
    polyref->next = poly_stack;
    poly_stack = polyref;
  }
}

/*
 * Decrement the reference kept in a option surface descriptor header. Call
 * the free proc if its no longer referenced.
 */
void surface_desc_unref(Surf_desc_hdr *surf_desc_hdr) {
  if (--surf_desc_hdr->ref_count <= 0) {
    if (surf_desc_hdr->ref_count < 0) {
      fprintf(stderr, "libsipp: negative surface descriptor reference count\n");
      abort();
    }
    if (surf_desc_hdr->free_func != NULL)
      (*surf_desc_hdr->free_func)(surf_desc_hdr);
  }
}

/*
 * Create a surface of all polygons in the polygon stack.
 * Empty the polygon stack afterwards.
 */
Surface *surface_create(void *surf_desc, Shader *shader) {
  Surface *surfref;

  if (poly_stack != NULL) {
    surfref = (Surface *)smalloc(sizeof(Surface));
    surfref->vertices = vertex_list;
    surfref->polygons = poly_stack;
    if (surface_desc_hdrs) {
      surfref->surf_desc_hdr = surf_desc;
      surfref->surface = SIPP_SURFP_HDR(void, surf_desc);
      surfref->surf_desc_hdr->ref_count++;
    } else {
      surfref->surf_desc_hdr = NULL;
      surfref->surface = surf_desc;
    }
    surfref->shader = shader;
#ifdef FFD
    surfref->ffd_desc_hdr = NULL;
    surfref->ffd_func = NULL;
    surfref->ffd_data = NULL;
#endif
    if (user_refcount) {
      surfref->ref_count = 1;
    } else {
      surfref->ref_count = 0;
    }
    vertex_list = NULL;
    first_vert = NULL;
    vhash_clear();
    poly_stack = NULL;
    first_vertex = 0;
    return surfref;
  } else
    return NULL;
}

/*
 * Create a surface to be shaded with the simple shader.
 */
Surface *surface_basic_create(double ambient, double red, double grn,
                              double blu, double specular, double c3,
                              double opred, double opgrn, double opblu) {
  Surf_desc_hdr *surf_desc_hdr;
  Surf_desc *surf_desc;
  Surface *surface;
  bool save;

  surf_desc_hdr = SIPP_SURF_HDR_ALLOC(Surf_desc);
  if (user_refcount) {
    surf_desc_hdr->ref_count = 1;
  } else {
    surf_desc_hdr->ref_count = 0;
  }
  surf_desc_hdr->free_func = free;
  surf_desc_hdr->client_data = NULL;

  surf_desc = SIPP_SURFP_HDR(Surf_desc, surf_desc_hdr);
  surf_desc->ambient = ambient;
  surf_desc->color.red = red;
  surf_desc->color.grn = grn;
  surf_desc->color.blu = blu;
  surf_desc->specular = specular;
  surf_desc->c3 = c3;
  surf_desc->opacity.red = opred;
  surf_desc->opacity.grn = opgrn;
  surf_desc->opacity.blu = opblu;

  save = sipp_surface_desc_headers(TRUE);
  surface = surface_create(surf_desc_hdr, basic_shader);
  sipp_surface_desc_headers(save);

  return surface;
}

/*
 * Set SURFACE to be shaded with the shading function SHADER
 * using the surface description SURF_DESC.
 */
void surface_set_shader(Surface *surface, void *surf_desc, Shader *shader) {

  if (surface != NULL) {
    if (surface->surf_desc_hdr != NULL) {
      surface_desc_unref(surface->surf_desc_hdr);
    }
    if (surface_desc_hdrs) {
      surface->surf_desc_hdr = surf_desc;
      surface->surface = SIPP_SURFP_HDR(void, surf_desc);
      surface->surf_desc_hdr->ref_count++;
    } else {
      surface->surf_desc_hdr = NULL;
      surface->surface = surf_desc;
    }
    surface->shader = shader;
  }
}

/*
 * Set SURFACE to be shaded with the simple shader.
 */
void surface_basic_shader(Surface *surface, double ambient, double red,
                          double grn, double blu, double specular, double c3,
                          double opred, double opgrn, double opblu) {
  Surf_desc_hdr *surf_desc_hdr;
  Surf_desc *surf_desc;
  bool save;

  surf_desc_hdr = SIPP_SURF_HDR_ALLOC(Surf_desc);
  surf_desc_hdr->ref_count = 0;
  surf_desc_hdr->free_func = free;
  surf_desc_hdr->client_data = NULL;

  surf_desc = SIPP_SURFP_HDR(Surf_desc, surf_desc_hdr);
  surf_desc->ambient = ambient;
  surf_desc->color.red = red;
  surf_desc->color.grn = grn;
  surf_desc->color.blu = blu;
  surf_desc->specular = specular;
  surf_desc->c3 = c3;
  surf_desc->opacity.red = opred;
  surf_desc->opacity.grn = opgrn;
  surf_desc->opacity.blu = opblu;

  save = sipp_surface_desc_headers(TRUE);
  surface_set_shader(surface, surf_desc_hdr, basic_shader);
  sipp_surface_desc_headers(save);
}

#ifdef FFD
void surface_set_ffd(Surface *surface, FFD_func *ffd_func, void *ffd_data) {
  if (surface != NULL) {
    if (surface->ffd_desc_hdr != NULL) {
      surface_desc_unref(surface->ffd_desc_hdr);
    }
    if (ffd_desc_hdrs) {
      surface->ffd_desc_hdr = ffd_data;
      surface->ffd_data = SIPP_SURFP_HDR(void, ffd_data);
      surface->ffd_desc_hdr->ref_count++;
    } else {
      surface->surf_desc_hdr = NULL;
      surface->ffd_data = ffd_data;
    }
    surface->ffd_func = ffd_func;
  }
}
#endif

/*
 * Copy a vertex list.  The copy of each vertex is remembered in the
 * HNEXT field of the original (which is unused once a surface exists),
 * so that copy_polygons() can translate vertex references.
 */
static Vertex *copy_vertices(Vertex *vp) {
  Vertex *head, *tail, *tmp;

  head = tail = NULL;
  for (; vp != NULL; vp = vp->next) {
    tmp = (Vertex *)smalloc(sizeof(Vertex));
    *tmp = *vp;
    tmp->next = NULL;
    tmp->hnext = NULL;
#ifdef FFD
    tmp->ffd_vertex = NULL; /* Allocated on demand when rendering */
#endif
    vp->hnext = tmp;
    if (tail == NULL) {
      head = tmp;
    } else {
      tail->next = tmp;
    }
    tail = tmp;
  }
  return head;
}

/*
 * Copy a list of polygons.
 */
static Polygon *copy_polygons(Polygon *pp, Surface *surface) {
  Polygon *tmp;
  int i;

  (void)surface;

  if (pp == NULL)
    return NULL;
  tmp = (Polygon *)smalloc(sizeof(Polygon));
  tmp->nvertices = pp->nvertices;
  tmp->vertex = (Vertex **)smalloc(tmp->nvertices * sizeof(Vertex *));
  for (i = 0; i < tmp->nvertices; i++) {
    /* copy_vertices() left the copy of each vertex in hnext. */
    tmp->vertex[i] = pp->vertex[i]->hnext;
  }
  tmp->next = copy_polygons(pp->next, surface);
  return tmp;
}

/*
 * Copy a surface. All polygons and vertices are copied but
 * the shader and surface description are the same as in the
 * original surfaces. Same goes for the ffd (if any).
 */
static Surface *surface_copy(Surface *surface) {
  Surface *newsurf;

  if (surface != NULL) {
    newsurf = (Surface *)smalloc(sizeof(Surface));
    memcpy(newsurf, surface, sizeof(Surface));
    newsurf->vertices = copy_vertices(surface->vertices);
    newsurf->polygons = copy_polygons(surface->polygons, newsurf);
    newsurf->ref_count = 1;
    if (newsurf->surf_desc_hdr != NULL) {
      newsurf->surf_desc_hdr->ref_count++;
    }
#ifdef FFD
    if (newsurf->ffd_desc_hdr != NULL) {
      newsurf->ffd_desc_hdr->ref_count++;
    }
#endif
    return newsurf;
  } else {
    return NULL;
  }
}

/*
 * Delete a vertex list.
 */
static void delete_vertices(Vertex **vlist) {
  Vertex *v, *next;

  for (v = *vlist; v != NULL; v = next) {
    next = v->next;
#ifdef FFD
    if (v->ffd_vertex != NULL) {
      sfree(v->ffd_vertex);
    }
#endif
    sfree(v);
  }
  *vlist = NULL;
}

/*
 * Delete a surface.
 */
void surface_unref(Surface *surface) {
  Polygon *polyref1, *polyref2;

  if (surface != NULL) {
    if (--surface->ref_count <= 0) {
      if (user_refcount && surface->ref_count < 0) {
        fprintf(stderr, "libsipp: negative surface reference count\n");
        abort();
      }
      polyref2 = surface->polygons;
      while (polyref2 != NULL) {
        sfree(polyref2->vertex);
        polyref1 = polyref2;
        polyref2 = polyref2->next;
        sfree(polyref1);
      }
      delete_vertices(&(surface->vertices));
      if (surface->surf_desc_hdr != NULL) {
        surface_desc_unref(surface->surf_desc_hdr);
      }
#ifdef FFD
      if (surface->ffd_desc_hdr != NULL) {
        surface_desc_unref(surface->ffd_desc_hdr);
      }
#endif
      sfree(surface);
    }
  }
}

/*
 * Create an empty object.
 */
Object *object_create(void) {
  Object *obj;

  obj = (Object *)smalloc(sizeof(Object));
  obj->num_surfaces = 0;
  obj->surfaces_size = 0;
  obj->surfaces = NULL;
  obj->num_sub_objs = 0;
  obj->sub_objs_size = 0;
  obj->sub_objs = NULL;
  MatCopy(&obj->transf, &ident_matrix);
  if (user_refcount) {
    obj->ref_count = 1;
  } else {
    obj->ref_count = 0;
  }

  return obj;
}

/*
 * Copy the top object in an object hierarchy.
 * The new object will reference the same
 * subobjects and surfaces as the original object.
 * copy_surfaces and copy_sub_objs indicate if surfaces
 * and subobjects are to be copied or just have references
 * incremented here.
 */
static Object *object_copy(Object *object, bool copy_surfaces,
                           bool copy_sub_objs) {
  Object *newobj;
  int idx;

  if (object == NULL) {
    return NULL;
  }

  newobj = (Object *)smalloc(sizeof(Object));
  memcpy(newobj, object, sizeof(Object));

  if (object->surfaces != NULL) {
    newobj->surfaces =
        (Surface **)smalloc(object->surfaces_size * sizeof(Surface *));
    if (copy_surfaces) {
      for (idx = 0; idx < object->num_surfaces; idx++) {
        newobj->surfaces[idx] = surface_copy(object->surfaces[idx]);
      }
    } else {
      for (idx = 0; idx < object->num_surfaces; idx++) {
        newobj->surfaces[idx] = object->surfaces[idx];
        newobj->surfaces[idx]->ref_count++;
      }
    }
  }

  if (object->sub_objs != NULL) {
    newobj->sub_objs =
        (Object **)smalloc(object->sub_objs_size * sizeof(Object *));
    if (copy_sub_objs) {
      for (idx = 0; idx < object->num_sub_objs; idx++) {
        newobj->sub_objs[idx] =
            object_copy(object->sub_objs[idx], copy_surfaces, TRUE);
        newobj->sub_objs[idx]->ref_count = 1;
      }
    } else {
      for (idx = 0; idx < object->num_sub_objs; idx++) {
        newobj->sub_objs[idx] = object->sub_objs[idx];
        newobj->sub_objs[idx]->ref_count++;
      }
    }
  }

  MatCopy(&newobj->transf, &ident_matrix);
  if (user_refcount) {
    newobj->ref_count = 1;
  } else {
    newobj->ref_count = 0;
  }
  return newobj;
}

/*
 * Copy the top node of an object hierarchy. The
 * subobjects and surface references will be the
 * same as in the original.
 */
Object *object_instance(Object *object) {
  /*
   * Don't copy surfaces or subobjects.
   */
  return object_copy(object, FALSE, FALSE);
}

/*
 * Copy an object hierarchy. The objects in
 * the new hierarchy will reference the same
 * surfaces as the object in
 * the old hierarchy, but all object nodes
 * will be duplicated.
 */
Object *object_dup(Object *object) {
  /*
   * Copy subobjects but not surfaces.
   */
  return object_copy(object, FALSE, TRUE);
}

/*
 * Copy an object hierarchy. All object nodes
 * and surfaces in the old hierarchy
 * will be duplicated.
 */
Object *object_deep_dup(Object *object) {
  /*
   * Copy surfaces and subobjects.
   */
  return object_copy(object, TRUE, TRUE);
}

/*
 * Recursively delete an object hierarchy. Reference
 * counts are decremented and if the result is zero
 * the recursion continues and the memory used is freed.
 * Don't allow deletion of the world.
 */
void object_delete(Object *o) { object_unref(o); } /* Backward compatibility */
void object_unref(Object *object) {
  int idx;

  if (object != NULL && object != sipp_world) {
    if (--object->ref_count <= 0) {
      if (user_refcount && object->ref_count < 0) {
        fprintf(stderr, "libsipp: negative object reference count\n");
        abort();
      }
      if (object->surfaces != NULL) {
        for (idx = 0; idx < object->num_surfaces; idx++) {
          surface_unref(object->surfaces[idx]);
        }
        sfree(object->surfaces);
      }

      if (object->sub_objs != NULL) {
        for (idx = 0; idx < object->num_sub_objs; idx++) {
          object_unref(object->sub_objs[idx]);
        }
        sfree(object->sub_objs);
      }
      sfree(object);
    }
  }
}

/*
 * Remove SUBOBJ as a subobject in OBJECT. SUBOBJ is only
 * removed from the list of subojects in OBJECT. If the
 * memory it uses should be freed, object_unref() must
 * be used. Returns FALSE if subobj is not in object.
 */
bool object_sub_subobj(Object *object, Object *subobj) {
  int idx;

  if (object == NULL || subobj == NULL) {
    return FALSE;
  }

  for (idx = 0; idx < object->num_sub_objs; idx++) {
    if (object->sub_objs[idx] == subobj)
      break;
  }
  if (idx == object->num_sub_objs)
    return FALSE;
  object->sub_objs[idx] = object->sub_objs[--object->num_sub_objs];

  object_unref(subobj);
  return TRUE;
}

/*
 * Add SUBOBJ as a subobject in OBJECT.
 */
void object_add_subobj(Object *object, Object *subobj) {
  if (object == NULL || subobj == NULL) {
    return;
  }

  if (object->num_sub_objs == object->sub_objs_size) {
    if (object->sub_objs == NULL) {
      object->sub_objs_size = SUB_OBJS_INIT_SIZE;
      object->sub_objs =
          (Object **)smalloc(SUB_OBJS_INIT_SIZE * sizeof(Object *));
    } else {
      object->sub_objs_size += SUB_OBJS_INCR_SIZE;
      object->sub_objs = (Object **)srealloc(
          object->sub_objs, object->sub_objs_size * sizeof(Object *));
    }
  }

  object->sub_objs[object->num_sub_objs++] = subobj;
  subobj->ref_count++;
}

/*
 * Remove SURFACE as a surface in OBJECT.
 * Returns FALSE if SURFACE is not in OBJECT.
 */
bool object_sub_surface(Object *object, Surface *surface) {
  int idx;

  if (object == NULL || surface == NULL || object->surfaces == NULL) {
    return FALSE;
  }

  for (idx = 0; idx < object->num_surfaces; idx++) {
    if (object->surfaces[idx] == surface)
      break;
  }

  if (idx == object->num_surfaces)
    return FALSE;
  object->surfaces[idx] = object->surfaces[--object->num_surfaces];

  surface_unref(surface);
  return TRUE;
}

/*
 * Add SURFACE to the list of surfaces belonging
 * to OBJECT.
 */
void object_add_surface(Object *object, Surface *surface) {
  if (object == NULL || surface == NULL) {
    return;
  }

  if (object->num_surfaces == object->surfaces_size) {
    if (object->surfaces == NULL) {
      object->surfaces_size = SURFACES_INIT_SIZE;
      object->surfaces =
          (Surface **)smalloc(SURFACES_INIT_SIZE * sizeof(Surface *));
    } else {
      object->surfaces_size += SURFACES_INCR_SIZE;
      object->surfaces = (Surface **)srealloc(
          object->surfaces, object->surfaces_size * sizeof(Surface *));
    }
  }

  object->surfaces[object->num_surfaces++] = surface;
  surface->ref_count++;
}

/*
 * Function to set flag indicating if surfaces descriptors have the header
 * structure used to keep reference counts.  This may be set differently
 * for different surfaces.  Returns the old value of the flag.
 */
bool sipp_surface_desc_headers(bool flag) {
  bool old = surface_desc_hdrs;

  surface_desc_hdrs = flag;
  return old;
}

#ifdef FFD
bool sipp_ffd_desc_headers(bool flag) {
  bool old = ffd_desc_hdrs;

  ffd_desc_hdrs = flag;
  return old;
}
#endif

bool sipp_user_refcount(bool flag) {
  bool old = user_refcount;

  user_refcount = flag;
  return old;
}

/*
 * Initialize the data structures.
 */
void objects_init(void) {
  vertex_list = NULL;
  first_vert = NULL;
  vhash_clear();
  vertex_stack = NULL;
  nvertices = 0;
  first_vertex = 0;
  poly_stack = NULL;
  sipp_world = object_create();
  sipp_user_refcount(FALSE);
  sipp_surface_desc_headers(FALSE);
#ifdef FFD
  sipp_ffd_desc_headers(FALSE);
#endif
}
