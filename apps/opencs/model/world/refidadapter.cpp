
#include "refidadapter.hpp"

#include "cassert"
#include <QVariant>

CSMWorld::RefIdAdapter::RefIdAdapter() {}

CSMWorld::RefIdAdapter::~RefIdAdapter() {}

QVariant CSMWorld::RefIdAdapter::getNestedData (const CSMWorld::RefIdColumn* column, const CSMWorld::RefIdData& data, int idnex, int subRowIndex, int subColIndex) const
{
    assert(false);
    return QVariant();
}

void CSMWorld::RefIdAdapter::setNestedData (const CSMWorld::RefIdColumn* column, CSMWorld::RefIdData& data, const QVariant& value, int index, int subRowIndex, int subColIndex) const
{
    assert(false);
}
