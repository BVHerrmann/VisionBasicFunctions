#include <QMouseEvent>
#include <QWheelEvent>

#include "qhalconwindow.h"
#include "qpainter.h"


QHalconWindow::QHalconWindow(QWidget* parent, long Width, long Height) : QWidget(parent)
, m_LastMousePos(-1, -1)
, m_WindowID(0)
{
  
}

// Resize the HALCON window whenever the QHalconWindow widget is resized
void QHalconWindow::resizeEvent(QResizeEvent* event)
{
  Q_UNUSED(event);
  SetWindowID();
}


void QHalconWindow::SetWindowID()
{
	if (m_WindowID == 0)
	{
		HalconCpp::OpenWindow(0, 0, width(), height(), (Hlong)winId(), "visible", "", &m_WindowID);
		//HalconCpp::SetWindowParam(m_WindowID, "graphics_stack", "true");
		HalconCpp::SetWindowParam(m_WindowID, "flush", "false");
		HalconCpp::SetPart(m_WindowID, 0, 0, height() - 1, width() - 1);
		// HalconCpp::HImage Image("byte", width(), height());
        //HalconCpp::GenImageProto(Image ,&Image, 0);
		//HalconCpp::DispImage(Image, m_WindowID);
	}
	HalconCpp::SetWindowExtents(m_WindowID, 0, 0, width(), height());
	HalconCpp::FlushBuffer(m_WindowID);
}


void QHalconWindow::PaintTextCameraNotActive()
{
	if (m_WindowID != 0)
	{
		QString TextMsg = tr("Camera Disable/Not Connected");
		HalconCpp::SetColor(m_WindowID, "red");
		HalconCpp::SetTposition(m_WindowID, 10, 10);
		HalconCpp::WriteString(m_WindowID, TextMsg.toLatin1().data());
		HalconCpp::FlushBuffer(m_WindowID);
	}
	
}


void QHalconWindow::GetPartFloat(double* row1, double* col1, double* row2, double* col2)
{ // to get float values from get_part, use HTuple parameters
  HalconCpp::HTuple trow1, tcol1, trow2, tcol2;
  HalconCpp::GetPart(m_WindowID, &trow1, &tcol1, &trow2, &tcol2);
  *row1 = trow1.D();
  *col1 = tcol1.D();
  *row2 = trow2.D();
  *col2 = tcol2.D();
}

void QHalconWindow::SetPartFloat(double row1, double col1, double row2,double col2)
{
  // convert the double values to HTuple. Otherwise the int variant of SetPart
  // is used this enables smooth movement and zooming even when zoomed in
  HalconCpp::SetPart(m_WindowID, HalconCpp::HTuple(row1), HalconCpp::HTuple(col1),HalconCpp::HTuple(row2), HalconCpp::HTuple(col2));
}


void QHalconWindow::mouseMoveEvent(QMouseEvent* event)
{
  if ((event->buttons() == Qt::LeftButton) && m_LastMousePos.x() != -1)
  {
    QPoint delta = m_LastMousePos - event->globalPos();

    // scale delta to image zooming factor
    double scalex = (m_LastCol2 - m_LastCol1 + 1) / (double)width();
    double scaley = (m_LastRow2 - m_LastRow1 + 1) / (double)height();
    try
    {
      // set new visible part
      SetPartFloat(m_LastRow1 + (delta.y() * scaley),
		  m_LastCol1 + (delta.x() * scalex),
		  m_LastRow2 + (delta.y() * scaley),
		  m_LastCol2 + (delta.x() * scalex));
      // initiate redraw ()
      HalconCpp::FlushBuffer(m_WindowID);
    }
    catch (const HalconCpp::HOperatorException&)
    {
      // this may happen, if the part image is moved outside the window
    }
  }
}


void QHalconWindow::mousePressEvent(QMouseEvent* event)
{  // save last mouse position and image part
  GetPartFloat(&m_LastRow1, &m_LastCol1, &m_LastRow2, &m_LastCol2);
  m_LastMousePos = event->globalPos();
}


void QHalconWindow::mouseReleaseEvent(QMouseEvent* event)
{
  Q_UNUSED(event);
  // unset reference mouse position
  m_LastMousePos = QPoint(-1, -1);
}


void QHalconWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
  Q_UNUSED(event);
  if (event->buttons() == Qt::LeftButton)
  { // reset image part
   	HalconCpp::SetPart(m_WindowID, 0, 0, -1, -1);
   	HalconCpp::FlushBuffer(m_WindowID);
  }
}


void QHalconWindow::wheelEvent(QWheelEvent* event)
{
  // event->delta() is a multiple of 120. For larger multiples, the user
  // rotated the wheel by multiple notches.
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
  std::function<int(void)>     qtAngleDelta = [&]() { return event->delta(); };
  std::function<QPointF(void)> qtPosition   = [&]() {
    QPointF p(event->x(), event->y());
    return p;
  };
#else
  std::function<int(void)> qtAngleDelta = [&]() {
    return event->angleDelta().y();
  };
  std::function<QPointF(void)> qtPosition = [&]() {
    return event->position();
  };
#endif
  int    num_notch = std::abs(qtAngleDelta()) / 120;
  double factor = (qtAngleDelta() > 0) ? std::sqrt(2.0) : 1.0 / std::sqrt(2.0);
  double centerRow, centerCol;
  while (num_notch > 1)
  {
    factor = factor * ((qtAngleDelta() > 0) ? std::sqrt(2.0) : 1.0 / std::sqrt(2.0));
    num_notch--;
  }
  // get zooming center
  HalconCpp::HTuple HcenterRow, HcenterCol;
  HalconCpp::ConvertCoordinatesWindowToImage(m_WindowID, qtPosition().y(), qtPosition().x(), &HcenterRow, &HcenterCol);

  centerRow = HcenterRow[0].D();
  centerCol = HcenterCol[0].D();

  // get current image part
  double row1, col1, row2, col2;
  GetPartFloat(&row1, &col1, &row2, &col2);
  // zoom around center
  double left    = centerRow - row1;
  double right   = row2 - centerRow;
  double top     = centerCol - col1;
  double buttom  = col2 - centerCol;
  double newRow1 = centerRow - left * factor;
  double newRow2 = centerRow + right * factor;
  double newCol1 = centerCol - top * factor;
  double newCol2 = centerCol + buttom * factor;
  try
  {
    SetPartFloat(newRow1, newCol1, newRow2, newCol2);
    HalconCpp::FlushBuffer(m_WindowID);
  }
  catch (const HalconCpp::HOperatorException&)
  {
    // this may happen, if the part is much too small or too big
  }
}
