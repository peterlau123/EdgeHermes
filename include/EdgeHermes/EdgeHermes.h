#pragma once

#include "utils/macros.h"

typedef void* EngineHandle;

bool EDGEHERMES_API init_engine();

bool EDGEHERMES_API load_model(EngineHandle hdl, const char* model_path);




