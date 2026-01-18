#include "gdextension_registration.hpp"

#include <gdextension_interface.h>

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "Debug.h"
#include "godot/GDCave.hpp"

using namespace godot;

void initialize_libgdcave(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
    return;
  }
  LOG_INFO("################# REGISTER GDCave");
  ClassDB::register_class<GDCave>();
}

void uninitialize_libgdcave(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
}

extern "C" {

GDExtensionBool GDE_EXPORT
libgdcave_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
               const GDExtensionClassLibraryPtr p_library,
               GDExtensionInitialization* r_initialization) {
  godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library,
                                                 r_initialization);

  init_obj.register_initializer(initialize_libgdcave);
  init_obj.register_terminator(uninitialize_libgdcave);
  init_obj.set_minimum_library_initialization_level(
      MODULE_INITIALIZATION_LEVEL_SCENE);

  return init_obj.init();
}
}
