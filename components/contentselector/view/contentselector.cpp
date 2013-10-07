#include "contentselector.hpp"

#include "../model/esmfile.hpp"
#include "lineedit.hpp"

#include <QSortFilterProxyModel>

#include <QMenu>
#include <QContextMenuEvent>
#include <QGridLayout>
#include <QMessageBox>
#include <assert.h>

#include "filewidget.hpp"
#include "adjusterwidget.hpp"
#include "textinputdialog.hpp"

#include <QDebug>

ContentSelectorView::ContentSelector *ContentSelectorView::ContentSelector::mInstance = 0;
QStringList ContentSelectorView::ContentSelector::mFilePaths;

void ContentSelectorView::ContentSelector::configure(QWidget *subject, unsigned char flags)
{
    assert(!mInstance);
    mInstance = new ContentSelector (subject, flags);
}

ContentSelectorView::ContentSelector& ContentSelectorView::ContentSelector::instance()
{

    assert(mInstance);
    return *mInstance;
}

ContentSelectorView::ContentSelector::ContentSelector(QWidget *parent, unsigned char flags) :
    QWidget(parent), mFlags (flags),
    mAdjusterWidget (0), mFileWidget (0),
    mIgnoreProfileSignal (false)
{

    ui.setupUi (this);

    parent->setLayout(new QGridLayout());
    parent->layout()->addWidget(this);

    buildContentModel();
    buildGameFileView();
    buildAddonView();
    buildNewAddonView();
    buildLoadAddonView();
    buildProfilesView();

    /*
    //mContentModel->sort(3);  // Sort by date accessed

*/
}

bool ContentSelectorView::ContentSelector::isFlagged(SelectorFlags flag) const
{
    return (mFlags & flag);
}

void ContentSelectorView::ContentSelector::buildContentModel()
{
    if (!isFlagged (Flag_Content))
            return;

    mContentModel = new ContentSelectorModel::ContentModel();

    if (mFilePaths.size()>0)
    {
        foreach (const QString &path, mFilePaths)
            mContentModel->addFiles(path);

        mFilePaths.clear();
    }

    ui.gameFileView->setCurrentIndex(-1);
    mContentModel->uncheckAll();
}

void ContentSelectorView::ContentSelector::buildGameFileView()
{
    if (!isFlagged (Flag_Content))
    {
        ui.gameFileView->setVisible(false);
        return;
    }

    ui.gameFileView->setVisible (true);

    mGameFileProxyModel = new QSortFilterProxyModel(this);
    mGameFileProxyModel->setFilterRegExp(QString::number((int)ContentSelectorModel::ContentType_GameFile));
    mGameFileProxyModel->setFilterRole (Qt::UserRole);
    mGameFileProxyModel->setSourceModel (mContentModel);

    ui.gameFileView->setPlaceholderText(QString("Select a game file..."));
    ui.gameFileView->setModel(mGameFileProxyModel);

    connect (ui.gameFileView, SIGNAL(currentIndexChanged(int)), this, SLOT (slotCurrentGameFileIndexChanged(int)));

    ui.gameFileView->setCurrentIndex(-1);
}

void ContentSelectorView::ContentSelector::buildAddonView()
{
    if (!isFlagged (Flag_Content))
    {
        ui.addonView->setVisible(false);
        return;
    }

    ui.addonView->setVisible (true);

    mAddonProxyModel = new QSortFilterProxyModel(this);
    mAddonProxyModel->setFilterRegExp (QString::number((int)ContentSelectorModel::ContentType_Addon));
    mAddonProxyModel->setFilterRole (Qt::UserRole);
    mAddonProxyModel->setDynamicSortFilter (true);
    mAddonProxyModel->setSourceModel (mContentModel);

    ui.addonView->setModel(mAddonProxyModel);

    connect(ui.addonView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotAddonTableItemClicked(const QModelIndex &)));
}

void ContentSelectorView::ContentSelector::buildProfilesView()
{
    if (!isFlagged (Flag_Profile))
    {
        ui.profileGroupBox->setVisible(false);
        return;
    }

    ui.profileGroupBox->setVisible (true);

    // Add the actions to the toolbuttons
    ui.newProfileButton->setDefaultAction (ui.newProfileAction);
    ui.deleteProfileButton->setDefaultAction (ui.deleteProfileAction);

    //enable ui elements
    ui.profilesComboBox->addItem ("Default");
    ui.profilesComboBox->setPlaceholderText (QString("Select a profile..."));

    if (!ui.profilesComboBox->isEnabled())
        ui.profilesComboBox->setEnabled(true);

    if (!ui.deleteProfileAction->isEnabled())
        ui.deleteProfileAction->setEnabled(true);

    //establish connections
    connect (ui.profilesComboBox, SIGNAL (currentIndexChanged(int)), this, SLOT(slotCurrentProfileIndexChanged(int)));
    connect (ui.profilesComboBox, SIGNAL (profileRenamed(QString,QString)), this, SIGNAL(signalProfileRenamed(QString,QString)));
    connect (ui.profilesComboBox, SIGNAL (signalProfileChanged(QString,QString)), this, SIGNAL(signalProfileChangedByUser(QString,QString)));
    connect (ui.profilesComboBox, SIGNAL (signalProfileTextChanged(QString)), this, SLOT (slotProfileTextChanged (QString)));

    ui.profileGroupBox->setVisible (true);
    ui.projectButtonBox->setVisible (false);
}

void ContentSelectorView::ContentSelector::buildLoadAddonView()
{
    if (!isFlagged (Flag_LoadAddon))
    {
        if (!isFlagged (Flag_NewAddon))
            ui.projectGroupBox->setVisible (false);

        return;
    }

    ui.projectGroupBox->setVisible (true);
    ui.projectCreateButton->setVisible (false);
    ui.projectGroupBox->setTitle ("");

    connect(ui.projectButtonBox, SIGNAL(accepted()), this, SIGNAL(accepted()));
    connect(ui.projectButtonBox, SIGNAL(rejected()), this, SIGNAL(rejected()));
}

void ContentSelectorView::ContentSelector::buildNewAddonView()
{
    if (!isFlagged (Flag_NewAddon))
    {
        if (!isFlagged (Flag_LoadAddon))
            ui.projectGroupBox->setVisible (false);

        return;
    }

    ui.projectGroupBox->setVisible (true);

    mFileWidget = new CSVDoc::FileWidget (this);
    mAdjusterWidget = new CSVDoc::AdjusterWidget (this);

    mFileWidget->setType(true);
    mFileWidget->extensionLabelIsVisible(false);

    ui.fileWidgetFrame->layout()->addWidget(mFileWidget);
    ui.adjusterWidgetFrame->layout()->addWidget(mAdjusterWidget);

    ui.projectButtonBox->setStandardButtons(QDialogButtonBox::Cancel);
    ui.projectButtonBox->addButton(ui.projectCreateButton, QDialogButtonBox::ActionRole);

    connect (mFileWidget, SIGNAL (nameChanged (const QString&, bool)),
        mAdjusterWidget, SLOT (setName (const QString&, bool)));

    connect (mAdjusterWidget, SIGNAL (stateChanged(bool)), this, SLOT (slotUpdateCreateButton(bool)));

    connect(ui.projectCreateButton, SIGNAL(clicked()), this, SIGNAL(accepted()));
    connect(ui.projectButtonBox, SIGNAL(rejected()), this, SIGNAL(rejected()));
}

void ContentSelectorView::ContentSelector::setGameFile(const QString &filename)
{
    int index = -1;

    if (!filename.isEmpty())
    {
        index = ui.gameFileView->findText(filename);

        //verify that the current index is also checked in the model
        mContentModel->setCheckState(filename, true);
    }

    ui.gameFileView->setCurrentIndex(index);
}

void ContentSelectorView::ContentSelector::clearCheckStates()
{
    mContentModel->uncheckAll();
}

void ContentSelectorView::ContentSelector::setCheckStates(const QStringList &list)
{
    if (list.isEmpty())
        return;

    foreach (const QString &file, list)
        mContentModel->setCheckState(file, Qt::Checked);
}

QString ContentSelectorView::ContentSelector::projectFilename() const
{
    QString filepath = "";

    if (mAdjusterWidget)
        filepath = QString::fromAscii(mAdjusterWidget->getPath().c_str());

    return filepath;
}

ContentSelectorModel::ContentFileList
        ContentSelectorView::ContentSelector::selectedFiles() const
{
    if (!mContentModel)
        return ContentSelectorModel::ContentFileList();

    return mContentModel->checkedItems();
}


void ContentSelectorView::ContentSelector::addFiles(const QString &path)
{
    // if the model hasn't been instantiated, queue the path
    if (!mInstance)
        mFilePaths.append(path);
    else
    {
        mInstance->mContentModel->addFiles(path);
        mInstance->ui.gameFileView->setCurrentIndex(-1);
    }
}

int ContentSelectorView::ContentSelector::getProfileIndex ( const QString &item) const
{
    return ui.profilesComboBox->findText (item);
}

void ContentSelectorView::ContentSelector::addProfile (const QString &item, bool setAsCurrent)
{
    if (item.isEmpty())
        return;

    QString previous = ui.profilesComboBox->currentText();

    if (ui.profilesComboBox->findText(item) == -1)
        ui.profilesComboBox->addItem(item);

    if (setAsCurrent)
        setProfile (ui.profilesComboBox->findText(item));
}

void ContentSelectorView::ContentSelector::setProfile(int index)
{
    //programmatic change requires second call to non-signalized "slot" since no signal responses
    //occur for programmatic changes to the profilesComboBox.
    if (index >= -1 && index < ui.profilesComboBox->count())
    {
        QString previous = ui.profilesComboBox->itemText(ui.profilesComboBox->currentIndex());
        QString current = ui.profilesComboBox->itemText(index);

        ui.profilesComboBox->setCurrentIndex(index);
    }
}

QString ContentSelectorView::ContentSelector::getProfileText() const
{
    return ui.profilesComboBox->currentText();
}

QAbstractItemModel *ContentSelectorView::ContentSelector::profilesModel() const
{
    return ui.profilesComboBox->model();
}

void ContentSelectorView::ContentSelector::slotCurrentProfileIndexChanged(int index)
{
    //don't allow deleting "Default" profile
    bool success = (ui.profilesComboBox->itemText(index) != "Default");

    ui.deleteProfileAction->setEnabled(success);
    ui.profilesComboBox->setEditEnabled(success);
}

void ContentSelectorView::ContentSelector::slotCurrentGameFileIndexChanged(int index)
{
    static int oldIndex = -1;

    QAbstractItemModel *const model = ui.gameFileView->model();
    QSortFilterProxyModel *proxy = dynamic_cast<QSortFilterProxyModel *>(model);

    if (proxy)
        proxy->setDynamicSortFilter(false);

    if (oldIndex > -1)
        model->setData(model->index(oldIndex, 0), false, Qt::UserRole + 1);

    oldIndex = index;

    model->setData(model->index(index, 0), true, Qt::UserRole + 1);

    if (proxy)
        proxy->setDynamicSortFilter(true);

    slotUpdateCreateButton(true);
}

void ContentSelectorView::ContentSelector::slotProfileTextChanged(const QString &text)
{
    QPushButton *opnBtn = ui.projectButtonBox->button(QDialogButtonBox::Open);

    if (opnBtn->isEnabled())
        opnBtn->setEnabled (false);
}

void ContentSelectorView::ContentSelector::slotAddonTableItemClicked(const QModelIndex &index)
{
    QAbstractItemModel *const model = ui.addonView->model();

    if (model->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    else
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
}

void ContentSelectorView::ContentSelector::slotUpdateCreateButton(bool)
{
    //enable only if a game file is selected and the adjuster widget is non-empty
    bool validGameFile = (ui.gameFileView->currentIndex() != -1);
    bool validFilename = false;

    if (mAdjusterWidget)
        validFilename = mAdjusterWidget->isValid();

    ui.projectCreateButton->setEnabled(validGameFile && validFilename);
}

void ContentSelectorView::ContentSelector::on_newProfileAction_triggered()
{
    TextInputDialog newDialog (tr("New Profile"), tr("Profile name:"), this);

    if (newDialog.exec() == QDialog::Accepted)
        emit signalAddNewProfile(newDialog.getText());
}

void ContentSelectorView::ContentSelector::on_deleteProfileAction_triggered()
{
    QString profile = ui.profilesComboBox->currentText();

    if (profile.isEmpty())
        return;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Delete Profile"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setText(tr("Are you sure you want to delete <b>%0</b>?").arg(profile));

    QAbstractButton *deleteButton =
    msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() != deleteButton)
        return;

    // Remove the profile from the combobox
    ui.profilesComboBox->removeItem(ui.profilesComboBox->findText(profile));

    //signal for removal from model
    emit signalProfileDeleted (profile);

    slotCurrentProfileIndexChanged(ui.profilesComboBox->currentIndex());
}
