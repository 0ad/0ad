#include "precompiled.h"

#include "Unit.h"
#include "Model.h"

CUnit::~CUnit() {
	delete m_Model;
}
