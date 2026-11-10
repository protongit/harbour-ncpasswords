#include "passwordlistmodel.h"

#include <algorithm>

PasswordListModel::PasswordListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PasswordListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant PasswordListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) {
        return QVariant();
    }
    const PasswordEntry &entry = m_entries.at(m_visible.at(index.row()));

    switch (role) {
    case IdRole:
        return entry.id;
    case LabelRole:
        return entry.label;
    case UsernameRole:
        return entry.username;
    case UrlRole:
        return entry.url;
    case FolderIdRole:
        return entry.folderId;
    case FolderLabelRole:
        return m_folderLabels.value(entry.folderId);
    case FavoriteRole:
        return entry.favorite;
    case HiddenRole:
        return entry.hidden;
    case TrashedRole:
        return entry.trashed;
    case StatusRole:
        return entry.status;
    case TagIdsRole:
        return entry.tagIds;
    case RevisionRole:
        return entry.revision;
    case HasPasswordRole:
        return !entry.password.isEmpty();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PasswordListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(IdRole, "passwordId");
    roles.insert(LabelRole, "label");
    roles.insert(UsernameRole, "username");
    roles.insert(UrlRole, "url");
    roles.insert(FolderIdRole, "folderId");
    roles.insert(FolderLabelRole, "folderLabel");
    roles.insert(FavoriteRole, "favorite");
    roles.insert(HiddenRole, "hidden");
    roles.insert(TrashedRole, "trashed");
    roles.insert(StatusRole, "status");
    roles.insert(TagIdsRole, "tagIds");
    roles.insert(RevisionRole, "revision");
    roles.insert(HasPasswordRole, "hasPassword");
    return roles;
}

int PasswordListModel::count() const
{
    return m_visible.size();
}

void PasswordListModel::setEntries(const QVector<PasswordEntry> &entries)
{
    m_entries = entries;
    rebuild();
}

void PasswordListModel::setFolderLabels(const QHash<QString, QString> &labels)
{
    m_folderLabels = labels;
    if (!m_visible.isEmpty()) {
        emit dataChanged(index(0), index(m_visible.size() - 1), QVector<int>() << FolderLabelRole);
    }
}

PasswordEntry PasswordListModel::entryById(const QString &id) const
{
    for (const PasswordEntry &entry : m_entries) {
        if (entry.id == id) {
            return entry;
        }
    }
    return PasswordEntry();
}

void PasswordListModel::setSearch(const QString &search)
{
    if (m_search == search) {
        return;
    }
    m_search = search.trimmed();
    rebuild();
}

void PasswordListModel::setFolderFilter(const QString &folderId)
{
    if (m_folder == folderId) {
        return;
    }
    m_folder = folderId;
    rebuild();
}

void PasswordListModel::setTagFilter(const QString &tagId)
{
    if (m_tag == tagId) {
        return;
    }
    m_tag = tagId;
    rebuild();
}

void PasswordListModel::setTrashMode(bool trash)
{
    if (m_trash == trash) {
        return;
    }
    m_trash = trash;
    rebuild();
}

void PasswordListModel::rebuild()
{
    beginResetModel();
    m_visible.clear();

    for (int i = 0; i < m_entries.size(); ++i) {
        const PasswordEntry &entry = m_entries.at(i);
        if (entry.trashed != m_trash) {
            continue;
        }
        if (!m_folder.isEmpty() && entry.folderId != m_folder) {
            continue;
        }
        if (!m_tag.isEmpty() && !entry.tagIds.contains(m_tag)) {
            continue;
        }
        if (!m_search.isEmpty()) {
            const QString needle = m_search.toLower();
            const bool matches = entry.label.toLower().contains(needle)
                    || entry.username.toLower().contains(needle)
                    || entry.url.toLower().contains(needle)
                    || entry.notes.toLower().contains(needle);
            if (!matches) {
                continue;
            }
        }
        m_visible.append(i);
    }

    std::sort(m_visible.begin(), m_visible.end(), [this](int left, int right) {
        const PasswordEntry &a = m_entries.at(left);
        const PasswordEntry &b = m_entries.at(right);
        if (a.favorite != b.favorite) {
            return a.favorite;
        }
        return QString::localeAwareCompare(a.label, b.label) < 0;
    });

    endResetModel();
    emit countChanged();
}
