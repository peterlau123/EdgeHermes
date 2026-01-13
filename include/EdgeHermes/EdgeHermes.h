#pragma once

#include "utils/macros.h"

typedef void* EngineHandle;

bool edgehermes_API init_engine();

bool edgehermes_API load_model(EngineHandle hdl, const char* model_path);



