#ifndef PASSWORDLISTMODEL_H
#define PASSWORDLISTMODEL_H

#include "models.h"

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

class PasswordListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        LabelRole,
        UsernameRole,
        UrlRole,
        FolderIdRole,
        FolderLabelRole,
        FavoriteRole,
        HiddenRole,
        TrashedRole,
        StatusRole,
        TagIdsRole,
        RevisionRole,
        HasPasswordRole
    };

    explicit PasswordListModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    int count() const;

    void setEntries(const QVector<PasswordEntry> &entries);
    void setFolderLabels(const QHash<QString, QString> &labels);

    const QVector<PasswordEntry> &entries() const { return m_entries; }
    PasswordEntry entryById(const QString &id) const;

    void setSearch(const QString &search);
    void setFolderFilter(const QString &folderId);
    void setTagFilter(const QString &tagId);
    void setTrashMode(bool trash);

signals:
    void countChanged();

private:
    void rebuild();

    QVector<PasswordEntry> m_entries;
    QVector<int> m_visible;
    QHash<QString, QString> m_folderLabels;
    QString m_search;
    QString m_folder;
    QString m_tag;
    bool m_trash = false;
};

#endif // PASSWORDLISTMODEL_H
