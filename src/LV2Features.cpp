/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>

#include "ingen/LV2Features.hpp"

using namespace std;

namespace Ingen {

LV2Features::LV2Features()
{
}

void
LV2Features::add_feature(SharedPtr<Feature> feature)
{
	_features.push_back(feature);
}

LV2Features::FeatureArray::FeatureArray(FeatureVector& features)
	: _features(features)
{
	_array = (LV2_Feature**)malloc(sizeof(LV2_Feature) * (features.size() + 1));
	_array[features.size()] = NULL;
	for (size_t i = 0; i < features.size(); ++i) {
		_array[i] = features[i].get();
	}
}

LV2Features::FeatureArray::~FeatureArray()
{
	free(_array);
}

SharedPtr<LV2Features::FeatureArray>
LV2Features::lv2_features(World* world, GraphObject* node) const
{
	FeatureArray::FeatureVector vec;
	for (Features::const_iterator f = _features.begin(); f != _features.end(); ++f) {
		SharedPtr<LV2_Feature> fptr = (*f)->feature(world, node);
		if (fptr) {
			vec.push_back(fptr);
		}
	}
	return SharedPtr<FeatureArray>(new FeatureArray(vec));
}

} // namespace Ingen