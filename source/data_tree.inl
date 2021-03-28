/*
Thinking of XML:
An object can contain:
  - any number of child objects
    or
  - a single value (string or number)
  - any number of value-string property pairs

For XCode format this is the same, although XCode files also contain commenting (in between even)
*/

struct data_tree_object_t {
  const char* name;
  const char* comment;
  bool has_children;  // If has children, then no value, else has value and no children
  bool is_array;
  union {
    const char* value;
    unsigned int first_child;  // !=0 if has childen, ==0 if no children
  };
  unsigned int next_sibling;  // !=0 if has more siblings, ==0 if no more siblings (siblings can be
                              // objects or parameters)

  unsigned int first_parameter;  // !=0 if has parameters, ==0 if no parameters
};

struct data_tree_t {
  struct data_tree_object_t* objects;
};

struct data_tree_api {
  struct data_tree_t (*create)();

  /* Create a new child object under 'parent_object' with name 'name'
   * parent_object == 0 means it's a top level object, usually not allowed more than once anyway.
   * name == 0 may mean the node is treated specially in output
   * returns index of created object
   */
  unsigned int (*create_object)(struct data_tree_t* tree, unsigned int parent_object,
                                const char* name);

  /* Find a child object under 'parent_object' with name 'name'. If child object is not found, then
   * it is created. parent_object == 0 means it's a top level object, usually not allowed more than
   * once anyway.
   *
   * returns index of created object
   */
  unsigned int (*get_or_create_object)(struct data_tree_t* tree, unsigned int parent_object,
                                       const char* name);

  /* Set the value of object at index 'object'. If the object has children they will be removed.
   */
  void (*set_object_value)(struct data_tree_t* tree, unsigned int object,
                           const char* object_value);

  /* Set the comment of object at index 'object'.
   */
  void (*set_object_comment)(struct data_tree_t* tree, unsigned int object, const char* comment);

  /* If 'object' already has a parameter with the same name, then overwrite the value, else add a
   * new parameter 'param_name' with value 'param_value'
   */
  void (*set_object_parameter)(struct data_tree_t* tree, unsigned int object,
                               const char* param_name, const char* param_value);
};

/***********************************************************************************************************************
 *                                             Implementation starts here
 ***********************************************************************************************************************/
struct data_tree_t dt_create() {
  struct data_tree_t out              = {0};
  struct data_tree_object_t empty_obj = {0};
  array_push(out.objects, empty_obj);
  return out;
}

unsigned int dt_create_object(struct data_tree_t* dt, unsigned int parent_object,
                              const char* name) {
  assert(dt);
  assert((parent_object == 0) || (parent_object < array_count(dt->objects)));

  struct data_tree_object_t obj = {(name ? cc_printf("%s", name) : NULL)};
  array_push(dt->objects, obj);

  unsigned int node_index               = array_count(dt->objects) - 1;
  struct data_tree_object_t* parent_obj = dt->objects + parent_object;
  if (parent_obj->has_children) {
    struct data_tree_object_t* sibling_obj = dt->objects + parent_obj->first_child;
    while (sibling_obj->next_sibling) {
      sibling_obj = dt->objects + sibling_obj->next_sibling;
    }
    sibling_obj->next_sibling = node_index;
  } else {
    parent_obj->has_children = true;
    parent_obj->first_child  = node_index;
  }

  return node_index;
}

unsigned int dt_get_or_create_object(struct data_tree_t* dt, unsigned int parent_object,
                                     const char* name) {
  assert(dt);
  assert((parent_object == 0) || (parent_object < array_count(dt->objects)));
  assert(name);

  // Search for a node with the same name
  struct data_tree_object_t* parent_obj  = dt->objects + parent_object;
  struct data_tree_object_t* sibling_obj = NULL;
  if (parent_obj->has_children) {
    unsigned int child_id = parent_obj->first_child;
    sibling_obj           = dt->objects + child_id;
    do {
      if (strcmp(sibling_obj->name, name) == 0) return child_id;
    } while ((child_id = sibling_obj->next_sibling) && (sibling_obj = (dt->objects + child_id)));
  }

  struct data_tree_object_t obj = {cc_printf("%s", name)};
  array_push(dt->objects, obj);

  unsigned int node_index = array_count(dt->objects) - 1;
  if (sibling_obj) {
    sibling_obj->next_sibling = node_index;
  } else {
    parent_obj->has_children = true;
    parent_obj->first_child  = node_index;
  }

  return node_index;
}

void dt_set_object_value(struct data_tree_t* dt, unsigned int object, const char* object_value) {
  struct data_tree_object_t* obj = dt->objects + object;
  obj->has_children              = false;
  if (object_value) {
    obj->value = cc_printf("%s", object_value);
  }
}

void dt_set_object_comment(struct data_tree_t* dt, unsigned int object, const char* comment) {
  if (comment) {
    struct data_tree_object_t* obj = dt->objects + object;
    obj->comment                   = cc_printf("%s", comment);
  }
}

void dt_set_object_parameter(struct data_tree_t* dt, unsigned int object, const char* param_name,
                             const char* param_value) {
  struct data_tree_object_t* obj       = dt->objects + object;
  struct data_tree_object_t* param_obj = NULL;
  if (obj->first_parameter) {
    param_obj = dt->objects + obj->first_parameter;
    do {
      if (strcmp(param_obj->name, param_name) == 0) {
        param_obj->value = cc_printf("%s", param_value);
        return;
      }
    } while (param_obj->next_sibling && (param_obj = dt->objects + param_obj->next_sibling));
  }

  struct data_tree_object_t new_param_obj = {0};
  new_param_obj.name                      = cc_printf("%s", param_name);
  new_param_obj.value                     = cc_printf("%s", param_value);

  if (param_obj) {
    param_obj->next_sibling = array_count(dt->objects);
  } else {
    obj->first_parameter = array_count(dt->objects);
  }

  array_push(dt->objects, new_param_obj);
}

const struct data_tree_api data_tree_api = {
    &dt_create,           &dt_create_object,      &dt_get_or_create_object,
    &dt_set_object_value, &dt_set_object_comment, &dt_set_object_parameter};
