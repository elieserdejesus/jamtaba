#ifndef _USER_NAME_LINE_EDIT_
#define _USER_NAME_LINE_EDIT_

#include <QLineEdit>

/**
 * @brief This class is used to 'undo' the QLineEdit changes when user type some invalid.
 * When some invalid text is typed the last valid value is used when the QLineEdit loses focus.
 */

class UserNameLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    UserNameLineEdit(QWidget *parent=0);

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private:
    QString lastValidUserName;

};

#endif
