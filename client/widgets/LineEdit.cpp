/*
 * QDigiDoc4
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "LineEdit.h"

#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionButton>

LineEdit::LineEdit(QWidget *parent)
	: QLineEdit(parent)
{}

void LineEdit::paintEvent(QPaintEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	// Workaround QTBUG-92199
	if(text().isEmpty() && (!placeholderText().isEmpty() || !placeholder.isEmpty()))
	{
		if(!placeholderText().isEmpty())
		{
			placeholder = placeholderText();
			setPlaceholderText({});
		}
		QLineEdit::paintEvent(event);

		QStyleOptionFrame opt;
		initStyleOption(&opt);
		QPainter p(this);
#ifdef Q_OS_WIN
		QRect lineRect = style()->subElementRect(QStyle::SE_LineEditContents, &opt, this);
#else
		QRect lineRect = rect();
#endif
		p.setClipRect(lineRect);
		QColor color = palette().color(QPalette::PlaceholderText);
		color.setAlpha(63);
		p.setPen(color);
		QFontMetrics fm = fontMetrics();
		int minLB = qMax(0, -fm.minLeftBearing());
		QRect ph = lineRect.adjusted(minLB + 3, 0, 0, 0);
		QString elidedText = fm.elidedText(placeholder, Qt::ElideRight, ph.width());
		p.drawText(ph, Qt::AlignVCenter, elidedText);
		return;
	}
#endif
	QLineEdit::paintEvent(event);
}
