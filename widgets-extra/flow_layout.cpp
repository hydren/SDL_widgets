/*
 * flow_layout.cpp
 *
 *  Created on: 28 de mar de 2016
 *      Author: carlosfaruolo
 */

#include "flow_layout.hpp"

#include "util.hpp"

using std::vector;

using WidgetsExtra::FlowLayout;

FlowLayout::FlowLayout(int x, int y, unsigned w, unsigned h)
: WidgetsExtra::Layout(x, y), maxWidth(w), maxHeight(h), spacing_h(DEFAULT_SPACING), spacing_v(DEFAULT_SPACING),
  alignment(BEFORE), orientation(HORIZONTAL)
{}

FlowLayout::FlowLayout(Point p)
: WidgetsExtra::Layout(p), maxWidth(0), maxHeight(0), spacing_h(DEFAULT_SPACING), spacing_v(DEFAULT_SPACING),
  alignment(BEFORE), orientation(HORIZONTAL)
{}

FlowLayout::~FlowLayout()
{}

void FlowLayout::pack()
{
	if(orientation == HORIZONTAL)
		packHorizontal();
	else
		packVertical();
}


void FlowLayout::packHorizontal()
{
	int currentStretchedSpacerSize = this->needsStretching()? this->computeFreeHorizontalSpaceOnLayout() / this->getStretchingElementsCount() : -1 ;
	Point prevPosition = position;
	foreach(Element*, component, vector<Element*>, this->components)
	{
		prevPosition.x += component->offset.x; //adds the offset to the position (to consume the space)
		Point actualPosition = prevPosition;
		actualPosition.y += component->offset.y + computeAlignment(component, actualPosition.y);
		component->setPosition(actualPosition); //sends a copy rect with the y axis offset'd by the component's y-offset
		prevPosition.x += spacing_h + (component->isStretched()? currentStretchedSpacerSize : component->getSize().w); //adds the current component width and global spacing
	}
}

void FlowLayout::packVertical()
{
	int currentStretchedSpacerSize = this->needsStretching()? this->computeFreeVerticalSpaceOnLayout() / this->getStretchingElementsCount() : -1 ;
	Point prevPosition = position;
	foreach(Element*, component, vector<Element*>, this->components)
	{
		prevPosition.y += component->offset.y; //adds the offset to the position (to consume the space)
		Point actualPosition = prevPosition;
		actualPosition.x += component->offset.x + computeAlignment(component, actualPosition.x);
		component->setPosition(actualPosition); //sends a copy rect with the y axis offset'd by the component's y-offset
		prevPosition.y += spacing_v + (component->isStretched()? currentStretchedSpacerSize : component->getSize().h); //adds the current component width and global spacing
	}
}

bool FlowLayout::needsStretching() const
{
	const_foreach(const Element*, component, vector<Element*>, this->components)
		if(component->isStretched())
			return true;

	return false;
}

unsigned FlowLayout::computeFreeHorizontalSpaceOnLayout() const
{
	int space = this->maxWidth == 0? SDL_GetVideoSurface()->w - this->position.x : this->maxWidth;
	const_foreach(const Element*, component, vector<Element*>, this->components)
	{
		if(not component->isStretched())
			space -= component->getSize().w; //subtract each non-stretched component size
	}

	space -= spacing_h * (1 + this->components.size()); //subtract each spacing between components

	return space > 0 ? space : 0;
}

unsigned FlowLayout::computeFreeVerticalSpaceOnLayout() const
{
	int space = this->maxHeight == 0? SDL_GetVideoSurface()->h - this->position.y : this->maxHeight;
	const_foreach(const Element*, component, vector<Element*>, this->components)
	{
		if(not component->isStretched())
			space -= component->getSize().h; //subtract each non-stretched component size
	}

	space -= spacing_v * (1 + this->components.size()); //subtract each spacing between components

	return space > 0 ? space : 0;
}

int FlowLayout::computeAlignment(Element* e, int pos) const
{
	if(this->alignment == BEFORE) return 0;

	int space;
	if(orientation == HORIZONTAL)
		space = this->maxHeight == 0? SDL_GetVideoSurface()->h - this->position.y : this->maxHeight;
	else
		space = this->maxWidth == 0? SDL_GetVideoSurface()->w - this->position.x : this->maxWidth;

	space -= e->getSize().h;

	if(this->alignment == AFTER)
		return space;

	else if(this->alignment == MIDDLE)
		return space * 0.5;

	//be careful to not fall into this one
	return 0;
}

unsigned FlowLayout::getStretchingElementsCount() const
{
	unsigned count = 0;
	const_foreach(const Element*, component, vector<Element*>, this->components)
		if(component->isStretched())
			count++;

	return count;
}
