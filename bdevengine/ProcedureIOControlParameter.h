#pragma once
#include "QtCore"


class ProcedureOutputControlParameter
{
public:
	ProcedureOutputControlParameter()
	{
		m_IsBool = false;
		m_TableIndexOPCUA = 0;
	}
	
public:
	QString  m_Name;//Name des Parameters
	QString  m_Type;//Type string real integer
	QString  m_Description;
	QVariant m_Value;//wert
	int      m_TableIndexOPCUA;
	bool     m_IsBool;
};


class ProcedureInputControlParameter
{
public:
	ProcedureInputControlParameter()
	{
		m_IsBool = false;
		m_TableIndexOPCUA = 0;
	}
	
public:
	QString  m_Name;//Name des Parameters
	QString  m_Type;//Type string real integer
	QString  m_Description;
	QVariant m_Value;//wert
	int      m_TableIndexOPCUA;
	bool     m_IsBool;
};


class ProcedureIOControlParameter
{
public:
	QString m_ProcedureName;
	QList<ProcedureInputControlParameter>  m_ListInputControlParameter;//Eingangsparameter
	QList<ProcedureOutputControlParameter> m_ListOutputControlParameter;//Ausgangsparameter(Ergebnnisse)
};
