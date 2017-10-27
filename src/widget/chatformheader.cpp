/*
    Copyright © 2017 by The qTox Project Contributors

    This file is part of qTox, a Qt-based graphical interface for Tox.

    qTox is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    qTox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with qTox.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "chatformheader.h"

#include "src/widget/maskablepixmapwidget.h"
#include "src/widget/style.h"
#include "src/widget/tool/callconfirmwidget.h"
#include "src/widget/tool/croppinglabel.h"
#include "src/widget/translator.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>
#include <QTextDocument>
#include <QToolButton>

static const QSize AVATAR_SIZE{40, 40};
static const QSize CALL_BUTTONS_SIZE{50, 40};
static const QSize TOOL_BUTTONS_SIZE{22, 18};
static const short HEAD_LAYOUT_SPACING = 5;
static const short MIC_BUTTONS_LAYOUT_SPACING = 4;
static const short BUTTONS_LAYOUT_HOR_SPACING = 4;

namespace  {
const QString ObjectName[] = {
    QString{},
    QStringLiteral("green"),
    QStringLiteral("red"),
    QStringLiteral("yellow"),
    QStringLiteral("yellow"),
};

const QString CallToolTip[] = {
    ChatFormHeader::tr("Can't start audio call"),
    ChatFormHeader::tr("Start audio call"),
    ChatFormHeader::tr("End audio call"),
    ChatFormHeader::tr("Cancel audio call"),
    ChatFormHeader::tr("Accept audio call"),
};

const QString VideoToolTip[] = {
    ChatFormHeader::tr("Can't start video call"),
    ChatFormHeader::tr("Start video call"),
    ChatFormHeader::tr("End video call"),
    ChatFormHeader::tr("Cancel video call"),
    ChatFormHeader::tr("Accept video call"),
};

const QString VolToolTip[] = {
    ChatFormHeader::tr("Sound can be disabled only during a call"),
    ChatFormHeader::tr("Unmute call"),
    ChatFormHeader::tr("Mute call"),
};

const QString MicToolTip[] = {
    ChatFormHeader::tr("Microphone can be muted only during a call"),
    ChatFormHeader::tr("Unmute microphone"),
    ChatFormHeader::tr("Mute microphone"),
};

}

template <class T, class Fun>
T* createButton(ChatFormHeader* self, const QSize& size, const QString& name, Fun slot)
{
    T* btn = new T();
    btn->setFixedSize(size);
    btn->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    const QString& path = QStringLiteral(":/ui/%1/%1.css").arg(name);
    btn->setStyleSheet(Style::getStylesheet(path));
    QObject::connect(btn, &QAbstractButton::clicked, self, slot);
    return btn;
}

ChatFormHeader::ChatFormHeader(QWidget* parent)
    : QWidget(parent)
    , mode{Mode::AV}
    , callState{CallButtonState::Disabled}
    , videoState{CallButtonState::Disabled}
    , volState{ToolButtonState::Disabled}
    , micState{ToolButtonState::Disabled}
{
    QHBoxLayout* headLayout = new QHBoxLayout();
    avatar = new MaskablePixmapWidget(this, AVATAR_SIZE, ":/img/avatar_mask.svg");

    nameLabel = new CroppingLabel();
    nameLabel->setObjectName("nameLabel");
    nameLabel->setMinimumHeight(Style::getFont(Style::Medium).pixelSize());
    nameLabel->setEditable(true);
    nameLabel->setTextFormat(Qt::PlainText);
    connect(nameLabel, &CroppingLabel::editFinished, this, &ChatFormHeader::onNameChanged);

    headTextLayout = new QVBoxLayout();
    headTextLayout->addStretch();
    headTextLayout->addWidget(nameLabel);
    headTextLayout->addStretch();

    micButton = createButton<QToolButton>(this, TOOL_BUTTONS_SIZE, "micButton", &ChatFormHeader::micMuteToggle);
    volButton = createButton<QToolButton>(this, TOOL_BUTTONS_SIZE, "volButton", &ChatFormHeader::volMuteToggle);
    callButton = createButton<QPushButton>(this, CALL_BUTTONS_SIZE, "callButton", &ChatFormHeader::callTriggered);
    videoButton = createButton<QPushButton>(this, CALL_BUTTONS_SIZE, "videoButton", &ChatFormHeader::videoCallTriggered);

    QVBoxLayout* micButtonsLayout = new QVBoxLayout();
    micButtonsLayout->setSpacing(MIC_BUTTONS_LAYOUT_SPACING);
    micButtonsLayout->addWidget(micButton, Qt::AlignTop | Qt::AlignRight);
    micButtonsLayout->addWidget(volButton, Qt::AlignTop | Qt::AlignRight);

    QGridLayout* buttonsLayout = new QGridLayout();
    buttonsLayout->addLayout(micButtonsLayout, 0, 0, 2, 1, Qt::AlignTop | Qt::AlignRight);
    buttonsLayout->addWidget(callButton, 0, 1, 2, 1, Qt::AlignTop);
    buttonsLayout->addWidget(videoButton, 0, 2, 2, 1, Qt::AlignTop);
    buttonsLayout->setVerticalSpacing(0);
    buttonsLayout->setHorizontalSpacing(BUTTONS_LAYOUT_HOR_SPACING);

    headLayout->addWidget(avatar);
    headLayout->addSpacing(HEAD_LAYOUT_SPACING);
    headLayout->addLayout(headTextLayout);
    headLayout->addLayout(buttonsLayout);

    setLayout(headLayout);

    updateButtonsView();
    Translator::registerHandler(std::bind(&ChatFormHeader::retranslateUi, this), this);
}

void ChatFormHeader::setName(const QString& newName)
{
    nameLabel->setText(newName);
    // for overlength names
    nameLabel->setToolTip(Qt::convertFromPlainText(newName, Qt::WhiteSpaceNormal));
}

void ChatFormHeader::setMode(ChatFormHeader::Mode mode)
{
    this->mode = mode;
    if (mode == Mode::None) {
        callButton->hide();
        videoButton->hide();
        volButton->hide();
        micButton->hide();
    }
}

template<class State>
void setStateToolTip(QAbstractButton* btn, State state, const QString toolTip[])
{
    const int index = static_cast<int>(state);
    btn->setToolTip(toolTip[index]);
}

void ChatFormHeader::retranslateUi()
{
    setStateToolTip(callButton, callState, CallToolTip);
    setStateToolTip(videoButton, videoState, VideoToolTip);
    setStateToolTip(micButton, micState, MicToolTip);
    setStateToolTip(volButton, volState, VolToolTip);
}

template<class State>
void setStateName(QAbstractButton* btn, State state)
{
    const int index = static_cast<int>(state);
    btn->setObjectName(ObjectName[index]);
    btn->setEnabled(index != 0);
}

void ChatFormHeader::updateButtonsView()
{
    setStateName(callButton, callState);
    setStateName(videoButton, videoState);
    setStateName(micButton, micState);
    setStateName(volButton, volState);
    retranslateUi();
    Style::repolish(this);
}

void ChatFormHeader::showOutgoingCall(bool video)
{
    CallButtonState& state = video ? videoState : callState;
    state = CallButtonState::Outgoing;
    updateButtonsView();
}

void ChatFormHeader::showCallConfirm(bool video)
{
    QWidget* btn = video ? videoButton : callButton;
    callConfirm = std::unique_ptr<CallConfirmWidget>(new CallConfirmWidget(btn));
    callConfirm->show();
    connect(callConfirm.get(), &CallConfirmWidget::accepted, this, &ChatFormHeader::callAccepted);
    connect(callConfirm.get(), &CallConfirmWidget::rejected, this, &ChatFormHeader::callRejected);
}

void ChatFormHeader::removeCallConfirm()
{
    callConfirm.reset(nullptr);
}

void ChatFormHeader::updateCallButtons(bool online, bool audio, bool video)
{
    const bool audioAvaliable = online && (mode & Mode::Audio);
    const bool videoAvaliable = online && (mode & Mode::Video);
    if (!audioAvaliable) {
        callState = CallButtonState::Disabled;
    } else if (video) {
        callState = CallButtonState::Disabled;
    } else if (audio) {
        callState = CallButtonState::InCall;
    } else {
        callState = CallButtonState::Avaliable;
    }

    if (!videoAvaliable) {
        videoState = CallButtonState::Disabled;
    } else if (video) {
        videoState = CallButtonState::InCall;
    } else if (audio) {
        videoState = CallButtonState::Disabled;
    } else {
        videoState = CallButtonState::Avaliable;
    }

    updateButtonsView();
}

void ChatFormHeader::updateMuteMicButton(bool active, bool inputMuted)
{
    micButton->setEnabled(active);
    if (active) {
        micState = inputMuted ? ToolButtonState::On : ToolButtonState::Off;
    } else {
        micState = ToolButtonState::Disabled;
    }

    updateButtonsView();
}

void ChatFormHeader::updateMuteVolButton(bool active, bool outputMuted)
{
    volButton->setEnabled(active);
    if (active) {
        volState = outputMuted ? ToolButtonState::On : ToolButtonState::Off;
    } else {
        volState = ToolButtonState::Disabled;
    }

    updateButtonsView();
}

void ChatFormHeader::setAvatar(const QPixmap &img)
{
    avatar->setPixmap(img);
}

QSize ChatFormHeader::getAvatarSize() const
{
    return QSize{avatar->width(), avatar->height()};
}

void ChatFormHeader::addWidget(QWidget* widget, int stretch, Qt::Alignment alignment)
{
    headTextLayout->addWidget(widget, stretch, alignment);
}

void ChatFormHeader::addLayout(QLayout* layout)
{
    headTextLayout->addLayout(layout);
}

void ChatFormHeader::addStretch()
{
    headTextLayout->addStretch();
}

void ChatFormHeader::onNameChanged(const QString& name)
{
    if (!name.isEmpty()) {
        nameLabel->setText(name);
        emit nameChanged(name);
    }
}
