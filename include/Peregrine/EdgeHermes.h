#pragma once

#include "utils/macros.h"

typedef void* EngineHandle;

bool PEREGRINE_API init_engine();

bool PEREGRINE_API load_model(EngineHandle hdl, const char* model_path);




