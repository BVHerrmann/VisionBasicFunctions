
#ifndef QHALCONWINDOW_H
#define QHALCONWINDOW_H


#include <QWidget>
#include "HalconCpp/HalconCpp.h"

class QHalconWindow : public QWidget
{
  Q_OBJECT

public:
  QHalconWindow(QWidget* parent = 0, long Width = 0, long Height = 0);
  HalconCpp::HTuple GetWindowID() { return m_WindowID; }
  void PaintTextCameraNotActive();
  void SetWindowID();
  //void paintEvent(QPaintEvent *e);

protected:
  void resizeEvent(QResizeEvent*);
  void mouseMoveEvent(QMouseEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void mouseDoubleClickEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  
private:
  void GetPartFloat(double* row1, double* col1, double* row2, double* col2);
  void SetPartFloat(double row1, double col1, double row2, double col2);
  QPoint   m_LastMousePos;
  double   m_LastRow1, m_LastCol1, m_LastRow2, m_LastCol2;
  HalconCpp::HTuple m_WindowID;
};

#endif // !QHALCONWINDOW_H
