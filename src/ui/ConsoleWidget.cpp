#include "ConsoleWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ConsoleWidget::ConsoleWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    auto *bar = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("运行输出"), this);
    title->setObjectName(QStringLiteral("SectionTitle"));
    auto *clearBtn = new QPushButton(QStringLiteral("清空"), this);
    auto *copyBtn = new QPushButton(QStringLiteral("复制全部"), this);
    clearBtn->setObjectName(QStringLiteral("GhostButton"));
    copyBtn->setObjectName(QStringLiteral("GhostButton"));
    bar->addWidget(title);
    bar->addStretch();
    bar->addWidget(copyBtn);
    bar->addWidget(clearBtn);
    root->addLayout(bar);

    m_output = new QPlainTextEdit(this);
    m_output->setObjectName(QStringLiteral("Console"));
    m_output->setReadOnly(true);
    m_output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_output->setMinimumHeight(200);
    root->addWidget(m_output, 1);

    auto *inputRow = new QHBoxLayout();
    auto *prompt = new QLabel(QStringLiteral("stdin >"), this);
    prompt->setObjectName(QStringLiteral("Muted"));
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(QStringLiteral("向运行中的进程输入内容后回车（可选）"));
    m_input->setEnabled(false);
    inputRow->addWidget(prompt);
    inputRow->addWidget(m_input, 1);
    root->addLayout(inputRow);

    connect(clearBtn, &QPushButton::clicked, this, &ConsoleWidget::clearOutput);
    connect(copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_output->toPlainText());
    });
    connect(m_input, &QLineEdit::returnPressed, this, [this]() {
        const QString text = m_input->text();
        m_input->clear();
        emit stdinSubmitted(text);
    });
}

void ConsoleWidget::appendText(const QString &text)
{
    if (text.isEmpty())
        return;
    m_output->moveCursor(QTextCursor::End);
    m_output->insertPlainText(text);
    m_output->moveCursor(QTextCursor::End);
}

void ConsoleWidget::clearOutput()
{
    m_output->clear();
}

void ConsoleWidget::setStdinEnabled(bool enabled)
{
    m_input->setEnabled(enabled);
}
