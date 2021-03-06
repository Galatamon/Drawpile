/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2008-2009 Calle Laakkonen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <QPainter>
#include "annotationitem.h"
#include "../shared/net/annotation.h"
#include "core/layer.h"
#include "core/tile.h"

namespace drawingboard {

/**
 * Return a new unique ID for an annotation. Note! These are used only
 * when in local mode! In network mode the server assigns all IDs,
 * including new ones for the annotations that existed when the
 * session was created.
 * @return ID number
 */
int AnnotationItem::nextId()
{
	static int ids=0;
	return ++ids;
}

AnnotationItem::AnnotationItem(int id, QGraphicsItem *parent)
	: QGraphicsItem(parent), id_(id), flags_(Qt::TextWordWrap), highlight_(false), forceborder_(false)
{
}

void AnnotationItem::growTopLeft(qreal x, qreal y)
{
	if(size_.width() - x <= 0) x = 0;
	if(size_.height() - y <= 0) y = 0;
	prepareGeometryChange();
	moveBy(x, y);
	size_ = QSizeF(size_.width() - x, size_.height() - y);
}

void AnnotationItem::growBottomRight(qreal x, qreal y)
{
	if(size_.width() + x <= 0) x = 0;
	if(size_.height() + y <= 0) y = 0;
	prepareGeometryChange();
	size_ = QSizeF(size_.width() + x, size_.height() + y);
}

/**
 * Note. Assumes point is inside the text box.
 */
AnnotationItem::Handle AnnotationItem::handleAt(const QPointF point)
{
	if(point.x() < HANDLE && point.y() < HANDLE)
		return RS_TOPLEFT;
	else if(point.x() > size_.width()-HANDLE && point.y() > size_.height()-HANDLE)
		return RS_BOTTOMRIGHT;
	return TRANSLATE;

}

void AnnotationItem::setHighlight(bool hl)
{
	bool old = highlight_;
	highlight_ = hl;
	if(hl != old)
		update();
}

/**
 * Normally a border is drawn only when an annotation lacks content
 * or when it is highlighted.
 */
void AnnotationItem::forceBorder(bool force)
{
	bool old = forceborder_;
	forceborder_ = force;
	if(force != old)
		update();
}

int AnnotationItem::justify() const {
	using protocol::Annotation;
	if((flags_ & Qt::AlignRight))
		return Annotation::RIGHT;
	if((flags_ & Qt::AlignHCenter))
		return Annotation::CENTER;
	if((flags_ & Qt::AlignJustify))
		return Annotation::FILL;
	return Annotation::LEFT;
}

void AnnotationItem::setOptions(const protocol::Annotation& a)
{
	QPointF newpos(a.rect.left(), a.rect.top());
	QSizeF newsize(a.rect.width(), a.rect.height());
	if(pos() != newpos)
		setPos(newpos);
	if(size_ != newsize) {
		prepareGeometryChange();
		size_ = newsize;
	}
	text_ = a.text;
	textcol_ = QColor(a.textcolor);
	textcol_.setAlpha(a.textalpha);
	bgcol_ = QColor(a.backgroundcolor);
	bgcol_.setAlpha(a.bgalpha);
	flags_ = Qt::TextWordWrap;
	switch(a.justify) {
		using protocol::Annotation;
		case Annotation::LEFT: flags_ |= Qt::AlignLeft; break;
		case Annotation::RIGHT: flags_ |= Qt::AlignRight; break;
		case Annotation::CENTER: flags_ |= Qt::AlignHCenter; break;
		case Annotation::FILL: flags_ |= Qt::AlignJustify; break;
	}
	font_.setBold(a.bold);
	font_.setItalic(a.italic);
	font_.setFamily(a.font);
	font_.setPixelSize(a.size);
	update();
}

void AnnotationItem::getOptions(protocol::Annotation& a)
{
	a.id = id_;
	a.rect = QRect(pos().toPoint(), size_.toSize());
	a.text = text();
	a.textcolor = textcol_.name();
	a.textalpha = textcol_.alpha();
	a.backgroundcolor = bgcol_.name();
	a.bgalpha = bgcol_.alpha();
	a.justify = justify();
	a.bold = bold();
	a.italic = italic();
	a.font = font();
	a.size = fontSize();
}

QRectF AnnotationItem::boundingRect() const {
	return QRectF(0, 0, size_.width(), size_.height());
}

/**
 * Render the annotation using a painter.
 * @param painter painter to render with
 * @param rect rectangle to which the annotation is rendered
 */
void AnnotationItem::render(QPainter *painter, const QRectF& rect) const
{
	painter->fillRect(rect, bgcol_);
	painter->setPen(textcol_);
	painter->setFont(font_);
	painter->drawText(rect, flags_, text_);
}

void AnnotationItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	QRectF rect (QPointF(), size_);
	render(painter, rect);

	// Draw a border. If background and text color are the same, draw
	// the border in negative.
	QColor border = textcol_;
	bool drawb = highlight_ || forceborder_ || text_.isEmpty();
	if(textcol_.rgb() == bgcol_.rgb()) {
		border = QColor(255-textcol_.red(), 255-textcol_.green(), 255-textcol_.blue());
		drawb = true;
	}
	border.setAlpha(255);

	QPen bpen(highlight_?Qt::DashLine:Qt::DotLine);
	if(drawb) {
		bpen.setColor(border);
		painter->setPen(bpen);
		painter->drawRect(rect);
	}

	// Draw resizing handles
	if(highlight_) {
		painter->setClipRect(QRectF(QPointF(), size_));
		painter->setPen(border);
		painter->setBrush(border);

		QPointF triangle[3] = {QPointF(0,0), QPointF(HANDLE,0), QPointF(0,HANDLE)};
		painter->drawConvexPolygon(triangle, 3);
		triangle[0] = QPointF(size_.width()-HANDLE, size_.height());
		triangle[1] = QPointF(size_.width(), size_.height());
		triangle[2] = QPointF(size_.width(),size_.height()-HANDLE); 
		painter->drawConvexPolygon(triangle, 3);
	}
}

/**
 * The annotation will be rendered on the new layer with an offset
 * so it can be merged simply by compositing whole tiles.
 * @param x this will be set to the tile x index
 * @param y this will be set to the tile y index
 */
dpcore::Layer *AnnotationItem::toLayer(int *x, int *y)
{
	using namespace dpcore;
	int xi = int(pos().x()/Tile::SIZE);
	int yi = int(pos().y()/Tile::SIZE);
	QPoint offset(int(pos().x() - xi*Tile::SIZE),
			int(pos().y() - yi*Tile::SIZE));
	QImage img(offset.x() + int(size_.width()),
			int(offset.y() + size_.height()), QImage::Format_ARGB32);
	img.fill(0);
	QPainter painter(&img);
	render(&painter, QRectF(offset, size_));
	if(x)
		*x = xi;
	if(y)
		*y = yi;
	return new Layer(0, -1, "", img);
}

}

