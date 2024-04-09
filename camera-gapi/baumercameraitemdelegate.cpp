#include "baumercameraitemdelegate.h"

#include <QtGui>
#include <QtWidgets>


BaumerCameraItemDelegate::BaumerCameraItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QWidget *BaumerCameraItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    // don't edit description
    if (index.column() != 1)
        return nullptr;

    QString value = index.model()->data(index, Qt::DisplayRole).toString();
    QList<QVariant> user_data = index.model()->data(index, Qt::UserRole).toList();
    bool writeable = user_data[0].toBool();

    // don't edit data that is not writeable
    if (!writeable)
        return nullptr;

    // init editor
    QWidget *editor = nullptr;

    int type = user_data[1].toInt();
    switch(type) {
    case QMetaType::Void:
        {
            QPushButton *button = new QPushButton(tr("Execute"), parent);
            connect(button, &QPushButton::clicked, this, static_cast<void (BaumerCameraItemDelegate::*)()>(&BaumerCameraItemDelegate::valueChanged));
            editor = button;
            break;
        }
    case QMetaType::QString:
        {
            QLineEdit *line_editor = new QLineEdit(parent);
            //lineEditor->setFrame(false);
            line_editor->setMaxLength(255);
            editor = line_editor;
            break;
        }
    case QMetaType::QStringList:
        {
            QComboBox *combo_box = new QComboBox(parent);
            //comboBox->setFrame(false);
            combo_box->addItems(user_data[2].toStringList());
            connect(combo_box, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, static_cast<void (BaumerCameraItemDelegate::*)( const QString &)>(&BaumerCameraItemDelegate::valueChanged));
            editor = combo_box;
            break;
        }
    case QMetaType::UInt:
    case QMetaType::LongLong:
        {
            QSpinBox *spin_box = new QSpinBox(parent);
            //spinBox->setFrame(false);
            spin_box->setMinimum(user_data[2].toInt());
            spin_box->setMaximum(user_data[3].toInt());
            connect(spin_box, static_cast<void (QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged), this, static_cast<void (BaumerCameraItemDelegate::*)(const QString &)>(&BaumerCameraItemDelegate::valueChanged));
            editor = spin_box;
            break;
        }
    case QMetaType::Float:
        {
            QDoubleSpinBox *spin_box = new QDoubleSpinBox(parent);
            //spinBox->setFrame(false);
            spin_box->setSingleStep(0.1);
            spin_box->setMinimum(user_data[2].toDouble());
            spin_box->setMaximum(user_data[3].toDouble());
            connect(spin_box, static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, static_cast<void (BaumerCameraItemDelegate::*)(const QString &)>(&BaumerCameraItemDelegate::valueChanged));
            editor = spin_box;
            break;
        }
    default:
        break;
    }

    return editor;
}

void BaumerCameraItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    // don't edit description
    if (index.column() != 1)
        return;

    QString value = index.model()->data(index, Qt::DisplayRole).toString();
    QList<QVariant> user_data = index.model()->data(index, Qt::UserRole).toList();

    // set value
    int type = user_data[1].toInt();
    switch(type) {
    case QMetaType::QString:
        {
            QLineEdit *line_editor = qobject_cast<QLineEdit *>(editor);
            if (line_editor) {
                line_editor->setText(value);
            }
            break;
        }
    case QMetaType::QStringList:
        {
            QComboBox *combo_box = qobject_cast<QComboBox *>(editor);
            if (combo_box) {
                combo_box->setCurrentIndex(user_data[2].toStringList().indexOf(value));
            }
            break;
        }
    case QMetaType::UInt:
    case QMetaType::LongLong:
        {
            QSpinBox *spin_box = qobject_cast<QSpinBox *>(editor);
            if (spin_box) {
                spin_box->setValue(value.toInt());
            }
            break;
        }
    case QMetaType::Float:
        {
            QDoubleSpinBox *spin_box = qobject_cast<QDoubleSpinBox *>(editor);
            if (spin_box) {
                spin_box->setValue(value.toDouble());
            }
            break;
        }
    default:
        break;
    }

}

void BaumerCameraItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    // don't edit description
    if (index.column() != 1)
        return;

    QList<QVariant> user_data = index.model()->data(index, Qt::UserRole).toList();
    bool writeable = user_data[0].toBool();

    // don't edit data that is not writeable
    if (!writeable)
        return;

    // get value
    QVariant value;

    int type = user_data[1].toInt();
    switch(type) {
    case QMetaType::QString:
        {
            QLineEdit *line_editor = qobject_cast<QLineEdit *>(editor);
            if (line_editor) {
                if (line_editor->isModified()) {
                    value = line_editor->text();
                }
            }
            break;
        }
    case QMetaType::QStringList:
        {
            QComboBox *combo_box = qobject_cast<QComboBox *>(editor);
            if (combo_box) {
                value = combo_box->currentText();
            }
            break;
        }
    case QMetaType::UInt:
    case QMetaType::LongLong:
        {
            QSpinBox *spin_box = qobject_cast<QSpinBox *>(editor);
            if (spin_box) {
                value = spin_box->value();
            }
            break;
        }
    case QMetaType::Float:
        {
            QDoubleSpinBox *spin_box = qobject_cast<QDoubleSpinBox *>(editor);
            if (spin_box) {
                value = spin_box->value();
            }
            break;
        }
    default:
        break;
    }

    if (!value.isNull()) {
        model->setData(index, value, Qt::DisplayRole);
    }
}

void BaumerCameraItemDelegate::valueChanged()
{
    QWidget *editor = qobject_cast<QWidget *>(sender());
    if (editor) {
        emit commitData(editor);
        emit closeEditor(editor);
    }
}

void BaumerCameraItemDelegate::valueChanged(const QString &text)
{
    Q_UNUSED(text);

    QWidget *editor = qobject_cast<QWidget *>(sender());
    if (editor) {
        emit commitData(editor);
    }
}
