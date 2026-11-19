#include "EditableChoices.h"
#include <QAbstractItemView>
#include <QCompleter>
#include <QEvent>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QThread>

namespace gc {
// Qt refuses to open a zero-row combo. Keep its popup reachable even after
// an unavailable provider reply, without making the placeholder selectable.
static void keepPopupAvailable(QComboBox *combo)
{
    if (combo->count()) return;
    auto *model = qobject_cast<QStandardItemModel *>(combo->model());
    if (!model) return;
    const auto draft = combo->currentText();
    const QSignalBlocker comboSignals(combo);
    const QSignalBlocker lineSignals(combo->lineEdit());
    combo->addItem(QStringLiteral("No choices loaded"));
    model->item(0)->setFlags(Qt::NoItemFlags);
    combo->setCurrentIndex(-1);
    combo->setEditText(draft);
}

bool EditableChoices::preserveLiteralFinish(QComboBox *combo)
{
    if (!combo || !combo->lineEdit() || combo->insertPolicy() != QComboBox::NoInsert) return false;
    // Qt 6.8.3 QComboBoxPrivate::editingFinished calls emitActivated for a
    // matching label before the native editor's finish handler runs. Its
    // QObjectPrivate connection uses the public combo as receiver context.
    return QObject::disconnect(combo->lineEdit(), &QLineEdit::editingFinished, combo, nullptr);
}

EditableChoices::EditableChoices(QComboBox *widget, Request request)
    : QObject(widget), combo(widget), requestChoices(std::move(request))
{
    keepPopupAvailable(widget);
    widget->installEventFilter(this);
    widget->view()->installEventFilter(this);
    connect(widget, &QComboBox::currentIndexChanged, this, [this] { invalidate(); });
    connect(widget, &QComboBox::editTextChanged, this, [this] { invalidate(); });
    typingRefresh.setSingleShot(true);
    typingRefresh.setInterval(150);
    connect(&typingRefresh, &QTimer::timeout, this, [this] {
        if (combo && combo->lineEdit()->hasFocus()) refresh();
    });
    // textEdited is user input only; native initialization and commits must not
    // start discovery. Debounce typing while retaining editable completions.
    connect(widget->lineEdit(), &QLineEdit::textEdited, this, [this] {
        if (!choicesLoadedForEdit) typingRefresh.start();
    });
    connect(widget->lineEdit(), &QLineEdit::editingFinished, this, [this] {
        choicesLoadedForEdit = false;
        typingRefresh.stop();
    });
}

QString EditableChoices::value() const
{
    const auto index = combo->currentIndex();
    // Qt may retain the old index while a user types new text. Do not replace
    // that draft with the old item data when a background reply arrives.
    if (index >= 0 && combo->currentText() == combo->itemText(index))
        return combo->itemData(index, Qt::UserRole).toString();
    return combo->currentText();
}

void EditableChoices::invalidate()
{
    ++generation;
    pending = 0;
}

quint64 EditableChoices::refresh()
{
    typingRefresh.stop();
    if (!combo || !combo->isEditable() || QThread::currentThread() != thread()) return 0;
    pending = ++generation;
    requestedValue = value();
    const auto id = pending;
    if (requestChoices) requestChoices(id);
    return id;
}

bool EditableChoices::apply(quint64 id, const QVector<ReferenceChoice> &choices)
{
    if (!combo || QThread::currentThread() != thread() || !combo->isEditable() ||
        id == 0 || id != pending || value() != requestedValue) return false;
    pending = 0; // A response is single-use, including unchanged lists.
    // A ready provider always includes the explicit (None) row. Let Qt filter
    // those choices locally until this edit finishes. Popup opens still refresh.
    choicesLoadedForEdit = !choices.isEmpty();
    bool same = combo->count() == choices.size();
    for (qsizetype i = 0; same && i < choices.size(); ++i) {
        const auto &choice = choices[i];
        same = combo->itemText(int(i)) == choice.label &&
            combo->itemData(int(i), Qt::UserRole).toString() == choice.value &&
            combo->itemData(int(i), Qt::ToolTipRole).toString() == choice.description &&
            combo->itemData(int(i), Qt::UserRole + 1).toString() == choice.entityId;
    }
    if (same) return true;

    const auto draft = combo->currentText();
    auto *line = combo->lineEdit();
    const int cursor = line->cursorPosition();
    const int selection = line->selectionStart();
    const int selectionLength = int(line->selectedText().size());
    const bool hadSelection = combo->currentIndex() >= 0 &&
        draft == combo->itemText(combo->currentIndex());
    int match = -1;
    for (qsizetype i = 0; i < choices.size(); ++i) {
        if (choices[i].value == requestedValue) { match = int(i); break; }
    }
    const auto nextText = match >= 0 ? choices[match].label : requestedValue;
    if (line->hasFocus() && hadSelection && nextText != draft) return false;
    if (!hadSelection && match >= 0 && choices[match].label != draft) match = -1;
    // Both emit independently. Blocking only the combo could emit a line-edit
    // change into the native editor while rebuilding the presentation.
    const QSignalBlocker comboSignals(combo);
    const QSignalBlocker lineSignals(line);
    combo->clear();
    for (const auto &choice : choices) {
        combo->addItem(choice.label, choice.value);
        combo->setItemData(combo->count() - 1, choice.description, Qt::ToolTipRole);
        combo->setItemData(combo->count() - 1, choice.entityId, Qt::UserRole + 1);
    }
    // Native property columns can be much narrower than the discovered labels.
    combo->view()->setMinimumWidth(qMax(combo->width(), qMin(600, combo->view()->sizeHintForColumn(0) + 32)));
    combo->setCurrentIndex(match);
    if (match < 0) combo->setEditText(hadSelection ? requestedValue : draft);
    keepPopupAvailable(combo);
    line->setCursorPosition(qMin(cursor, int(line->text().size())));
    if (selection >= 0) line->setSelection(selection, selectionLength);
    // A user may already be typing when the first on-demand list arrives.
    // Refresh completion explicitly: blocked presentation signals cannot do it.
    if (line->hasFocus() && !combo->view()->isVisible() && !draft.isEmpty() && combo->completer()) {
        combo->completer()->setCompletionPrefix(line->text());
        combo->completer()->complete();
    }
    return true;
}

bool EditableChoices::eventFilter(QObject *target, QEvent *event)
{
    if (!combo) return false;
    if (target == combo->view() && event->type() == QEvent::Show)
        refresh();
    return false;
}
}
