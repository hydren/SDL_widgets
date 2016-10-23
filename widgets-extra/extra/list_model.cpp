/*
 * list_model.cpp
 *
 *  Created on: 20 de jun de 2016
 *      Author: carlosfaruolo
 */

#include "list_model.hpp"

using std::vector;
using std::string;

using WidgetsExtra::DefaultUIListModel;


DefaultUIListModel::DefaultUIListModel()
: data()
{}

DefaultUIListModel::DefaultUIListModel(const vector<string>& data)
: data(data)
{}

string DefaultUIListModel::getStringfiedElementAt(unsigned index)
{
	return this->data.at(index);
}

unsigned DefaultUIListModel::size()
{
	return this->data.size();
}

void DefaultUIListModel::updateData(const vector<string>& data)
{
	this->data = data;
}

void DefaultUIListModel::updateData(void* data)
{
	this->updateData(*static_cast<vector<string>*>(data));
}

void* DefaultUIListModel::getData()
{
	return &this->data;
}
