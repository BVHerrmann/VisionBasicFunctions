#pragma once


class ResultBufferAndImage
{
public:
	ResultBufferAndImage()
	{
		ClearResults();
	}
	void ClearResults()
	{
		m_ResultID=0;
		m_ImageDataIndex=0;
		m_ProcedureIndex=0;
		m_ResultString="";
		m_ResultList.clear();
		m_ResultsValid = false;
		m_ObjectFound = false;
		ErrorText = "NoError";
	}
	ResultBufferAndImage& operator=(const ResultBufferAndImage& other)
	{
		if (this != &other)
		{
			m_ResultID         = other.m_ResultID;
			m_ImageDataIndex   = other.m_ImageDataIndex;
			m_ProcedureIndex   = other.m_ProcedureIndex;
			m_ResultString     = other.m_ResultString;
			m_ResultsValid     = other.m_ResultsValid;
			m_ObjectFound      = other.m_ObjectFound;
			ErrorText          = other.ErrorText;
		}
		return *this;
	}

public:
	bool m_ResultsValid;
	bool m_ObjectFound;
	int m_ResultID;
	int m_ImageDataIndex;
	int m_ProcedureIndex;
	QString m_ResultString;
	QList<double> m_ResultList;
	QString ErrorText;
};
Q_DECLARE_METATYPE(ResultBufferAndImage);
