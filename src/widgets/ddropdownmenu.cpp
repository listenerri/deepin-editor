/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common/utils.h"
#include "../common/settings.h"
#include "ddropdownmenu.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <DApplication>
#include <QPainter>
#include <DSettingsOption>
#include <QDebug>
#include <DFontSizeManager>
#include <DLabel>
#include <DApplicationHelper>
#include <QtSvg/QSvgRenderer>

using namespace Dtk::Core;

QVector<QPair<QString,QStringList>> DDropdownMenu::sm_groupEncodeVec;

DDropdownMenu::DDropdownMenu(QWidget *parent)
    : QFrame(parent)
    , m_pToolButton(new DToolButton(this))
    , m_menu(new DMenu)
{
    //设置toobutton属性
    m_pToolButton->setFocusPolicy(Qt::StrongFocus);
    m_pToolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_pToolButton->setArrowType(Qt::NoArrow);
    m_pToolButton->setFixedHeight(28);
    m_pToolButton->installEventFilter(this);
    //this->installEventFilter(this);
    //设置图标
    QString theme =  (DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Background).lightness() < 128) ? "dark" : "light";
    QString arrowSvgPath = QString(":/images/arrow_%1.svg").arg(theme);
    //装换图片
    int scaled =qApp->devicePixelRatio() == 1.25 ? 2 : 1;
    QSvgRenderer svg_render(arrowSvgPath);
    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);
    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));
    m_arrowPixmap = pixmap;
    m_pToolButton->setIcon(createIcon());

    //设置字体
    int fontsize =DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T9);
    m_font.setPixelSize(fontsize);
    m_font.setFamily("SourceHanSansSC-Normal");

     //添加布局
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_pToolButton);
    layout->setContentsMargins(0,0,0,0);
    this->setLayout(layout);

    connect(this, &DDropdownMenu::requestContextMenu, this, [=] (bool bClicked){
        if(bClicked){
            //如果鼠标点击清除ｆｏｃｕｓ
            m_pToolButton->clearFocus();
        }
        QPoint center = this->mapToGlobal(this->rect().center());
        int menuHeight = m_menu->sizeHint().height();
        int menuWidth = m_menu->sizeHint().width();
        center.setY(center.y() - menuHeight - this->rect().height()/2);
        center.setX(center.x() - menuWidth / 2);
        m_menu->move(center);
        m_menu->exec();
        if(bClicked){
            //如果鼠标点击清除ｆｏｃｕｓ
            m_pToolButton->clearFocus();
            QEvent event(QEvent::HoverLeave);
            QApplication::sendEvent(m_pToolButton,&event);
        }
    });

    //设置字体自适应大小
    //设置界面大小根据内容大小自适应 梁卫东 2020.7.7
    connect(qApp,&DApplication::fontChanged,this,&DDropdownMenu::OnFontChangedSlot);
}

DDropdownMenu::~DDropdownMenu() {}

void DDropdownMenu::setFontEx(const QFont& font)
{
    m_pToolButton->setFont(font);
    m_font = font;
}



void DDropdownMenu::setCurrentAction(QAction *pAct)
{
    if(pAct){
        QList<QAction*> menuList = m_menu->actions();
        pAct->setChecked(true);
        for (int i = 0; i < menuList.size(); i++) {
            QList<QAction*> acts = menuList[i]->menu()->actions();
            for (int j = 0; j < acts.size(); j++) {
                if(acts[j] != pAct) acts[j]->setChecked(false);
            }
        }
        setText(pAct->text());
    }
}

void DDropdownMenu::setCurrentTextOnly(const QString &name)
{
   QList<QAction*> menuList = m_menu->actions();

   for (int i = 0; i < menuList.size(); i++) {
       if(menuList[i]->menu()){
           QList<QAction*> acts = menuList[i]->menu()->actions();
           if(acts.size() == 0) continue;
           for (int j = 0; j < acts.size(); j++) {
           if(acts[j]->text() != name){
               acts[j]->setCheckable(false);
               acts[j]->setChecked(false);
           }
           else{
               acts[j]->setCheckable(true);
               acts[j]->setChecked(true);
           }
        }
      }
   }

   setText(name);
}

void DDropdownMenu::setText(const QString &text)
{
    m_text = text;
    //重新绘制icon　设置宽度
    m_pToolButton->setIcon(createIcon());
}

void DDropdownMenu::setMenu(DMenu *menu)
{
    if (m_menu) {
        delete m_menu;
    }
    m_menu = menu;
}

void DDropdownMenu::setTheme(const QString &theme)
{
    QString arrowSvgPath = QString(":/images/arrow_%1.svg").arg(theme);
    //装换图片
    int scaled =qApp->devicePixelRatio() == 1.25 ? 2 : 1;
    QSvgRenderer svg_render(arrowSvgPath);

    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);

    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));

    m_arrowPixmap = pixmap;
    m_pToolButton->setIcon(createIcon());
}

void DDropdownMenu::setChildrenFocus(bool ok)
{
    if(ok)  m_pToolButton->setFocusPolicy(Qt::StrongFocus);
    else  {
        m_pToolButton->clearFocus();
        m_pToolButton->setFocusPolicy(Qt::NoFocus);
    }
}

DToolButton *DDropdownMenu::getButton()
{
    return m_pToolButton;
}

DDropdownMenu *DDropdownMenu::createEncodeMenu()
{
    DDropdownMenu *m_pEncodeMenu = new DDropdownMenu();
    DMenu* m_pMenu = new DMenu();
    if(sm_groupEncodeVec.isEmpty()){
        QFile file(":/encodes/encodes.ini");
        QString data;
        if(file.open(QIODevice::ReadOnly))
        {
           data = QString::fromUtf8(file.readAll());
           file.close();
        }

        QTextStream readStream(&data,QIODevice::ReadOnly);
        while (!readStream.atEnd()) {
            QString group = readStream.readLine();
            QString key = group.mid(1,group.length()-2);
            QString encodes = readStream.readLine();
            QString value = encodes.mid(8,encodes.length()-2);
            sm_groupEncodeVec.append(QPair<QString,QStringList>(key,value.split(",")));

            QStringList list = value.split(",");
            QMenu* groupMenu = new QMenu(QObject::tr(key.toLocal8Bit().data()));
             foreach(QString var,list)
             {
               QAction *act= groupMenu->addAction(QObject::tr(var.toLocal8Bit().data()));
               if(act->text() == "UTF-8") {
                   m_pEncodeMenu->m_pActUtf8 = act;
                   act->setChecked(true);
               }else {
                   act->setCheckable(false);
               }

             }

            m_pMenu->addMenu(groupMenu);
        }
    }else {

        int cnt = sm_groupEncodeVec.size();
        for (int i = 0;i < cnt;i++) {
            QMenu* groupMenu = new QMenu(QObject::tr(sm_groupEncodeVec[i].first.toLocal8Bit().data()));
             foreach(QString var,sm_groupEncodeVec[i].second)
             {
               QAction *act= groupMenu->addAction(QObject::tr(var.toLocal8Bit().data()));
               if(act->text() == "UTF-8") {
                   m_pEncodeMenu->m_pActUtf8 = act;
                   act->setCheckable(true);
                   act->setChecked(true);
               }else {
                   act->setCheckable(false);
               }
             }

            m_pMenu->addMenu(groupMenu);
        }
    }

    connect(m_pMenu, &DMenu::triggered, m_pEncodeMenu,[m_pEncodeMenu](QAction *action) {
        //编码内容改变触发内容改变和信号发射 梁卫东 2020.7.7
        if (m_pEncodeMenu->m_text != action->text()) {
            emit m_pEncodeMenu->currentActionChanged(action);
        }
    });

    m_pEncodeMenu->setText("UTF-8");
    m_pEncodeMenu->setMenu(m_pMenu);

    return  m_pEncodeMenu;
}

DDropdownMenu *DDropdownMenu::createHighLightMenu()
{
    DDropdownMenu *m_pHighLightMenu = new DDropdownMenu();
    DMenu *m_pMenu = new DMenu;
    QAction *noHlAction = m_pMenu->addAction(tr("None"));
    noHlAction->setCheckable(true);

    QActionGroup* m_pActionGroup = new QActionGroup(m_pMenu);
    m_pActionGroup->setExclusive(true);
    m_pActionGroup->addAction(noHlAction);


    DMenu *pSubMenu = nullptr;
    QString currentGroup;


    bool intel = true;
    for (KSyntaxHighlighting::Definition def : m_pHighLightMenu->m_Repository.definitions()) {

        if(def.translatedName()=="Intel x86 (NASM)"&&intel)
        {
            intel = false;
            continue;
        }
        if (def.isHidden()) {
            continue;
        }

        if (currentGroup != def.section()) {
            currentGroup = def.section();
            pSubMenu = m_pMenu->addMenu(def.translatedSection());
        }

        if (!pSubMenu) {
            continue;
        }

        QAction* action = pSubMenu->addAction(def.translatedName());
        action->setCheckable(true);
        action->setText(def.name());
        m_pActionGroup->addAction(action);

    }

    connect(m_pActionGroup, &QActionGroup::triggered, m_pHighLightMenu, [m_pHighLightMenu] (QAction *action) {
        const auto defName = action->text();
        const auto def = m_pHighLightMenu->m_Repository.definitionForName(defName);
        if (def.isValid() && m_pHighLightMenu->m_text != action->text()) {
            emit m_pHighLightMenu->currentActionChanged(action);
        }

    });


    m_pHighLightMenu->setText(tr("None"));
    m_pHighLightMenu->setMenu(m_pMenu);
    return m_pHighLightMenu;
}


QIcon DDropdownMenu::createIcon()
{
    int scaled = 1;
    if(devicePixelRatioF() == 1.25) scaled = 2;

    DPalette dpalette  = DApplicationHelper::instance()->palette(m_pToolButton);
    QColor textColor;

    QPixmap arrowPixmap;

    if(m_bPressed){
        textColor = dpalette.color(DPalette::Highlight);
        QString color = textColor.name(QColor::HexRgb);
        arrowPixmap = setSvgColor(color);
    }else {
        textColor = dpalette.color(DPalette::WindowText);
        arrowPixmap = m_arrowPixmap;
    }


    //根据字体大小设置icon大小
    //height 30    width QFontMetrics fm(font()) fm.width(text)+40;
    int fontWidth = QFontMetrics(m_font).width(m_text)+20;
    int fontHeight = QFontMetrics(m_font).height();
    int iconW = 8;
    int iconH = 5;


    int totalWidth = fontWidth + iconW + 20;
    int totalHeigth = 28;
    m_pToolButton->setFixedSize(totalWidth,totalHeigth);
    m_pToolButton->setIconSize(QSize(totalWidth,totalHeigth));


    QPixmap icon(QSize(totalWidth,totalHeigth)*scaled);
    icon.setDevicePixelRatio(scaled);
    icon.fill(Qt::transparent);

    QPainter painter(&icon);
    painter.setFont(m_font);
    painter.setPen(textColor);
    painter.drawText(QRectF(10,(totalHeigth-fontHeight)/2,fontWidth,fontHeight),m_text);

    //arrowPixmap=arrowPixmap.scaled(iconW,iconH,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    //qDebug()<<"==================="<<arrowPixmap.rect().height();
    painter.drawPixmap(QRectF(fontWidth,(totalHeigth-iconH)/2,iconW,iconH),arrowPixmap,arrowPixmap.rect());

    painter.end();
    return icon;
}

void DDropdownMenu::OnFontChangedSlot(const QFont &font)
{
    m_font = font;
    int fontsize =DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T8);
    m_font.setPixelSize(fontsize);
    m_pToolButton->setIcon(createIcon());
}


bool DDropdownMenu::eventFilter(QObject *object, QEvent *event)
{
    if(object == m_pToolButton){
        if(event->type() == QEvent::KeyPress){
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            //QString key = Utils::getKeyshortcut(keyEvent);
            if(keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Space)        //按下enter展开列表
            {
                Q_EMIT requestContextMenu(false);
                return true;
            }
            return false;
        }

        if(event->type() == QEvent::MouseButtonPress){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                m_bPressed = true;
                //重新绘制icon 点击改变前景色
                m_pToolButton->setIcon(createIcon());
                return true;
            }

            if(mouseEvent->button() == Qt::RightButton){
                return true;
            }
        }



        if(event->type() == QEvent::MouseButtonRelease){

            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                m_bPressed = false;
                m_pToolButton->setIcon(createIcon());
                Q_EMIT requestContextMenu(true);
                m_pToolButton->clearFocus();
            }
            return true;
        }

    }

    return QFrame::eventFilter(object,event);
}

QPixmap DDropdownMenu::setSvgColor(QString color)
{
    //设置图标颜色
    QString path = QString(":/images/arrow_dark.svg");
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    QDomDocument doc;
    doc.setContent(data);
    file.close();
    QDomElement elem = doc.documentElement();
    SetSVGBackColor(elem, "fill", color);

    //装换图片
    int scaled =qApp->devicePixelRatio() == 1.25 ? 2 : 1;
    QSvgRenderer svg_render(doc.toByteArray());

    QPixmap pixmap(QSize(8,5)*scaled);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scaled);

    QPainter painter(&pixmap);
    svg_render.render(&painter,QRect(0,0,8,5));

    return pixmap;
}

void DDropdownMenu::SetSVGBackColor(QDomElement &elem, QString strattr, QString strattrval)
{

    if (elem.tagName().compare("g") == 0 && elem.attribute("id").compare("color") == 0)
    {
        QString before_color = elem.attribute(strattr);
        elem.setAttribute(strattr, strattrval);
    }
    for (int i = 0; i < elem.childNodes().count(); i++)
    {
        if (!elem.childNodes().at(i).isElement()) continue;
        QDomElement element = elem.childNodes().at(i).toElement();
        SetSVGBackColor(element, strattr, strattrval);
    }
}
