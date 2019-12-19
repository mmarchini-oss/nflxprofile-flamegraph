// reading a text file
#include <emscripten.h>
#include <string.h>
#include <stdlib.h>
#include "nflxprofile.pb-c.h"
#include "deps/parson/parson.h"

typedef enum _LibType {
  kNone = 0
} LibType;

struct _Node;

struct NodeList {
  struct _Node* node;
  struct NodeList *next;
};

typedef struct _Node {
  char* name;
  /* LibType libtype; */
  /* unsigned long value; */
  struct NodeList* children;
  JSON_Value* json_value;
  JSON_Value* json_children;
} Node;

static char* emptyLib = "";

Node* init_node(char* function_name) {
  Node* node = calloc(1, sizeof(Node));
  node->name = function_name;
  /* strcpy(node->name, function_name); */

  node->json_value = json_value_init_object();
  JSON_Object *object = json_value_get_object(node->json_value);
  json_object_set_value(object, "name", json_value_init_string_no_copy(function_name));
  json_object_set_value(object, "libtype", json_value_init_string_no_copy(emptyLib));
  json_object_set_number(object, "value", 0);
  node->json_children = json_value_init_array();
  json_object_set_value(object, "children", node->json_children);
  return node;
}

void free_node(Node* node) {
  if (node->children != NULL) {
    for (struct NodeList* child = node->children; child != NULL; child = child->next) {
      free_node(child->node);
    }
    free(node->children);
  }

  free(node);
}

Node* get_child(Node* node, Nflxprofile__StackFrame* stack_frame) {
  Node* child = NULL;
  struct NodeList* children = node->children;
  for (struct NodeList* entry = children; entry != NULL; entry = entry->next) {
    if (strcmp(entry->node->name, stack_frame->function_name) == 0) {
      child = entry->node;
      break;
    }
  }
  if (child == NULL) {
    child = init_node(stack_frame->function_name);
    /* strcpy(child->name, stack_frame->function_name); */
    node->children = calloc(1, sizeof(struct NodeList));
    node->children->node = child;
    node->children->next = children;

    JSON_Array* json_children = json_value_get_array(node->json_children);
    json_array_append_value(json_children, child->json_value);
  }
  return child;
}

char* to_string(JSON_Value* value) {
  char *serialized_string = NULL;
  serialized_string = json_serialize_to_string(value);
  json_value_free(value);
  return serialized_string;
}

EMSCRIPTEN_KEEPALIVE
char* parse(void *buf, unsigned long len) {
  char * result;
  Nflxprofile__Profile* profile;
  uint64_t sample;

  profile = nflxprofile__profile__unpack(NULL, len, buf);
  free(buf);
  if (profile == NULL) {
    return NULL;
  }

  /* Node* root = { "root", kNone, 0, NULL }; */
  Node* root = init_node("root");
  Node* current_node;
  Node* child;

  for (int index=0; index < profile->n_samples - 1; index++) {
    sample = profile->samples[index];

    Nflxprofile__Profile__Node* node = NULL;
    for (int i=0; i < profile->n_nodes; i++) {
      if (profile->nodes[i]->key == sample) {
        node = profile->nodes[i]->value;
        break;
      }
    }
    if (node == NULL) {
      return NULL;
    }

    current_node = root;
    for (int stack_i=0; stack_i < node->n_stack; stack_i++) {
      current_node = get_child(current_node, node->stack[stack_i]);
    }

    JSON_Object *object = json_value_get_object(current_node->json_value);
    json_object_set_number(object, "value", json_object_get_number(object, "value") + 1);
  }
  JSON_Value* json_value = root->json_value;
  free_node(root);
  result = to_string(json_value);
  nflxprofile__profile__free_unpacked(profile, NULL);
  return result;
}
