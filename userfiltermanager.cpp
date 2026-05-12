#include "userfiltermanager.h"

UserFilterManager::UserFilterManager(QObject *parent)
    : QObject(parent)
{
}

bool UserFilterManager::loadFromFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = QString("Cannot open file: %1").arg(path);
        return false;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        if (error) *error = QString("JSON parse error: %1").arg(parseErr.errorString());
        return false;
    }

    if (!doc.isObject()) {
        if (error) *error = "Root must be a JSON object";
        return false;
    }

    QJsonObject root = doc.object();
    if (root["version"].toString().isEmpty()) {
        if (error) *error = "Missing 'version' field";
        return false;
    }

    QJsonArray arr = root["filters"].toArray();
    if (arr.isEmpty()) {
        if (error) *error = "No filters found in 'filters' array";
        return false;
    }

    QVector<UserFilter> parsed = parseFilterArray(arr, error);

    if (parsed.isEmpty()) {
        return false;
    }

    m_filters = parsed;
    emit filtersChanged();
    return true;
}

void UserFilterManager::clear()
{
    m_filters.clear();
    emit filtersChanged();
}

const QVector<UserFilter> &UserFilterManager::filters() const
{
    return m_filters;
}

int UserFilterManager::activeFilterCount() const
{
    int count = 0;
    for (const auto &f : m_filters) {
        if (f.enabled && f.isValid) count++;
    }
    return count;
}

QHash<int, QColor> UserFilterManager::applyToEntries(
    const QVector<DltAnalyzerInterface::LogEntry> &entries) const
{
    QHash<int, QColor> result;
    for (const auto &e : entries) {
        for (const auto &f : m_filters) {
            if (!f.enabled || !f.isValid) continue;
            if (matchesEntry(f, e)) {
                result.insert(e.index, f.color);
                break;
            }
        }
    }
    return result;
}

bool UserFilterManager::matchesEntry(
    const UserFilter &f, const DltAnalyzerInterface::LogEntry &e) const
{
    if (!f.domain.isEmpty() && f.domain != e.domain)
        return false;

    if (!f.levels.isEmpty() && !f.levels.contains(e.level))
        return false;

    for (const QString &field : f.fields) {
        QString value;
        if (field == "payload") value = e.payload;
        else if (field == "apid") value = e.apid;
        else if (field == "ctid") value = e.ctid;
        else if (field == "ecu") value = e.ecu;
        else continue;

        if (f.regex.match(value).hasMatch())
            return true;
    }

    return false;
}

QVector<UserFilter> UserFilterManager::parseFilterArray(const QJsonArray &arr, QString *error)
{
    QVector<UserFilter> result;
    result.reserve(arr.size());

    for (int i = 0; i < arr.size(); ++i) {
        if (!arr[i].isObject()) {
            if (error) *error = QString("Filter #%1: not an object").arg(i);
            continue;
        }

        UserFilter f = parseSingleFilter(arr[i].toObject(), i, error);
        if (f.isValid) {
            result.append(f);
        }
    }

    return result;
}

UserFilter UserFilterManager::parseSingleFilter(const QJsonObject &obj, int idx, QString *error)
{
    UserFilter f;

    QString label = obj["label"].toString().trimmed();
    if (label.isEmpty()) {
        if (error) *error = QString("Filter #%1: missing or empty 'label'").arg(idx);
        return f;
    }
    f.label = label;

    QString pattern = obj["pattern"].toString();
    if (pattern.isEmpty()) {
        if (error) *error = QString("Filter '%1': missing 'pattern'").arg(label);
        return f;
    }
    f.regex = QRegularExpression(pattern);
    if (!f.regex.isValid()) {
        if (error) *error = QString("Filter '%1': invalid regex '%2': %3")
            .arg(label, pattern, f.regex.errorString());
        return f;
    }

    QJsonArray fieldsArr = obj["fields"].toArray();
    if (fieldsArr.isEmpty()) {
        if (error) *error = QString("Filter '%1': missing or empty 'fields'").arg(label);
        return f;
    }
    for (const auto &v : fieldsArr)
        f.fields.append(v.toString().toLower());
    static const QStringList validFields = {"payload", "apid", "ctid", "ecu"};
    for (const auto &field : f.fields) {
        if (!validFields.contains(field)) {
            if (error) *error = QString("Filter '%1': invalid field '%2'").arg(label, field);
            return f;
        }
    }

    QString colorStr = obj["color"].toString();
    if (colorStr.isEmpty()) {
        if (error) *error = QString("Filter '%1': missing 'color'").arg(label);
        return f;
    }
    f.color = QColor(colorStr);
    if (!f.color.isValid()) {
        if (error) *error = QString("Filter '%1': invalid color '%2'").arg(label, colorStr);
        return f;
    }

    QJsonArray levelsArr = obj["level"].toArray();
    for (const auto &v : levelsArr)
        f.levels.append(v.toString().toLower());

    f.domain = obj["domain"].toString().toLower();
    f.enabled = obj.value("enabled").toBool(true);
    f.isValid = true;

    return f;
}
