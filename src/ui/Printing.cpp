#include "ui/Printing.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPageSize>
#include <QPainter>
#include <QPrinter>
#include <QPrinterInfo>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <cmath>

namespace {

constexpr double kDefaultMarginMm = 12.7; // 1270 centésimas de mm

QPageSize defaultPageSize()
{
    const QPrinterInfo info = QPrinterInfo::defaultPrinter();
    if (!info.isNull() && info.defaultPageSize().isValid()) return info.defaultPageSize();
    return QLocale().measurementSystem() == QLocale::MetricSystem ? QPageSize(QPageSize::A4) : QPageSize(QPageSize::Letter);
}

} // namespace

PageSettings::PageSettings()
    : layout(defaultPageSize(), QPageLayout::Portrait,
             QMarginsF(kDefaultMarginMm, kDefaultMarginMm, kDefaultMarginMm, kDefaultMarginMm), QPageLayout::Millimeter)
{
}

namespace printing {

Placement place(QSizeF printable, QSizeF physical, const PageSettings &s)
{
    Placement pl;
    if (printable.isEmpty() || physical.isEmpty()) return pl;
    if (s.fit) {
        const QSizeF total(std::max(1, s.fitWide) * printable.width(), std::max(1, s.fitTall) * printable.height());
        const double k = std::min(total.width() / physical.width(), total.height() / physical.height());
        pl.size = physical * k;
    } else {
        pl.size = physical * (std::max(1, s.percent) / 100.0);
    }
    pl.pagesX = std::max(1, int(std::ceil(pl.size.width() / printable.width() - 1e-6)));
    pl.pagesY = std::max(1, int(std::ceil(pl.size.height() / printable.height() - 1e-6)));
    const QSizeF total(pl.pagesX * printable.width(), pl.pagesY * printable.height());
    pl.offset = QPointF(s.centerH ? (total.width() - pl.size.width()) / 2 : 0.0,
                        s.centerV ? (total.height() - pl.size.height()) / 2 : 0.0);
    return pl;
}

bool render(QPrinter *printer, const QImage &image, double dpiX, double dpiY, const PageSettings &s)
{
    QPainter p;
    if (!p.begin(printer)) return false;
    const double res = printer->resolution();
    const QSizeF area = printer->pageLayout().paintRectPixels(int(res)).size();
    const QSizeF physical(image.width() / dpiX * res, image.height() / dpiY * res);
    const Placement pl = place(area, physical, s);
    p.setRenderHint(QPainter::SmoothPixmapTransform, pl.size.width() < image.width());
    bool first = true;
    for (int py = 0; py < pl.pagesY; ++py)
        for (int px = 0; px < pl.pagesX; ++px) {
            if (!first && !printer->newPage()) return false;
            first = false;
            const QPointF origin(px * area.width(), py * area.height());
            p.save();
            p.setClipRect(QRectF(QPointF(0, 0), area));
            p.translate(-origin);
            p.drawImage(QRectF(pl.offset, pl.size), image);
            p.restore();
        }
    return p.end();
}

// ---- Diálogo «Page setup» ----

namespace {

class PagePreview : public QWidget {
public:
    PagePreview(const PageSettings &s, QSize imagePx, double dpi) : m_s(s), m_img(imagePx), m_dpi(dpi) { setMinimumSize(150, 150); }
    void update(const PageSettings &s) { m_s = s; QWidget::update(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const QSizeF paper = m_s.layout.fullRect(QPageLayout::Point).size();
        if (paper.isEmpty()) return;
        const double k = std::min((width() - 16) / paper.width(), (height() - 16) / paper.height());
        const QRectF page(QPointF((width() - paper.width() * k) / 2, (height() - paper.height() * k) / 2), paper * k);
        p.fillRect(page.translated(3, 3), palette().shadow());
        p.fillRect(page, Qt::white);
        p.setPen(palette().color(QPalette::WindowText));
        p.drawRect(page);
        const QRectF paint = m_s.layout.paintRect(QPageLayout::Point);
        const QRectF area(page.topLeft() + paint.topLeft() * k, paint.size() * k);
        p.setPen(QPen(Qt::gray, 1, Qt::DotLine));
        p.drawRect(area);
        const QSizeF physical(m_img.width() / m_dpi * 72.0 * k, m_img.height() / m_dpi * 72.0 * k);
        const Placement pl = place(area.size(), physical, m_s);
        p.setClipRect(area);
        p.fillRect(QRectF(area.topLeft() + pl.offset, pl.size), QColor(0, 95, 184, 90));
    }

private:
    PageSettings m_s;
    QSize m_img;
    double m_dpi;
};

} // namespace

bool pageSetupDialog(PageSettings &s, QSize imagePx, double dpi)
{
    QDialog dlg;
    dlg.setWindowTitle(QObject::tr("Page Setup"));
    const bool metric = QLocale().measurementSystem() == QLocale::MetricSystem;
    const QPageLayout::Unit unit = metric ? QPageLayout::Millimeter : QPageLayout::Inch;
    PageSettings work = s;
    auto *preview = new PagePreview(work, imagePx, dpi > 0 ? dpi : 96.0);

    // Papel
    auto *paperBox = new QGroupBox(QObject::tr("Paper"));
    auto *paperForm = new QFormLayout(paperBox);
    auto *sizeCombo = new QComboBox;
    const QPageSize::PageSizeId ids[] = {QPageSize::Letter, QPageSize::Legal, QPageSize::Executive, QPageSize::A3,
                                         QPageSize::A4, QPageSize::A5, QPageSize::B4, QPageSize::B5, QPageSize::Tabloid};
    for (QPageSize::PageSizeId id : ids) sizeCombo->addItem(QPageSize(id).name(), int(id));
    int current = sizeCombo->findData(int(work.layout.pageSize().id()));
    if (current < 0) { sizeCombo->addItem(work.layout.pageSize().name(), int(work.layout.pageSize().id())); current = sizeCombo->count() - 1; }
    sizeCombo->setCurrentIndex(current);
    auto *sourceCombo = new QComboBox;
    sourceCombo->addItem(QObject::tr("Automatically Select"));
    paperForm->addRow(QObject::tr("Size:"), sizeCombo);
    paperForm->addRow(QObject::tr("Source:"), sourceCombo);

    // Orientación
    auto *orientBox = new QGroupBox(QObject::tr("Orientation"));
    auto *orientLay = new QVBoxLayout(orientBox);
    auto *portrait = new QRadioButton(QObject::tr("Portrait"));
    auto *landscape = new QRadioButton(QObject::tr("Landscape"));
    (work.layout.orientation() == QPageLayout::Portrait ? portrait : landscape)->setChecked(true);
    orientLay->addWidget(portrait);
    orientLay->addWidget(landscape);

    // Márgenes
    auto *marginBox = new QGroupBox(metric ? QObject::tr("Margins (millimeters)") : QObject::tr("Margins (inches)"));
    auto *marginGrid = new QGridLayout(marginBox);
    const QMarginsF m = work.layout.margins(unit);
    auto spin = [&](double v) {
        auto *sp = new QDoubleSpinBox;
        sp->setDecimals(metric ? 1 : 2);
        sp->setRange(0, metric ? 200 : 8);
        sp->setSingleStep(metric ? 1 : 0.1);
        sp->setValue(v);
        return sp;
    };
    auto *left = spin(m.left()), *right = spin(m.right()), *top = spin(m.top()), *bottom = spin(m.bottom());
    marginGrid->addWidget(new QLabel(QObject::tr("Left:")), 0, 0);
    marginGrid->addWidget(left, 0, 1);
    marginGrid->addWidget(new QLabel(QObject::tr("Right:")), 0, 2);
    marginGrid->addWidget(right, 0, 3);
    marginGrid->addWidget(new QLabel(QObject::tr("Top:")), 1, 0);
    marginGrid->addWidget(top, 1, 1);
    marginGrid->addWidget(new QLabel(QObject::tr("Bottom:")), 1, 2);
    marginGrid->addWidget(bottom, 1, 3);

    // Centrado
    auto *centerBox = new QGroupBox(QObject::tr("Centering"));
    auto *centerLay = new QHBoxLayout(centerBox);
    auto *centerH = new QCheckBox(QObject::tr("Horizontal"));
    auto *centerV = new QCheckBox(QObject::tr("Vertical"));
    centerH->setChecked(work.centerH);
    centerV->setChecked(work.centerV);
    centerLay->addWidget(centerH);
    centerLay->addWidget(centerV);

    // Escala
    auto *scaleBox = new QGroupBox(QObject::tr("Scaling"));
    auto *scaleGrid = new QGridLayout(scaleBox);
    auto *adjust = new QRadioButton(QObject::tr("Adjust to:"));
    auto *fitTo = new QRadioButton(QObject::tr("Fit to:"));
    auto *percent = new QSpinBox;
    percent->setRange(10, 400);
    percent->setValue(work.percent);
    auto *wide = new QSpinBox;
    wide->setRange(1, 99);
    wide->setValue(work.fitWide);
    auto *tall = new QSpinBox;
    tall->setRange(1, 99);
    tall->setValue(work.fitTall);
    (work.fit ? fitTo : adjust)->setChecked(true);
    scaleGrid->addWidget(adjust, 0, 0);
    scaleGrid->addWidget(percent, 0, 1);
    scaleGrid->addWidget(new QLabel(QObject::tr("% normal size")), 0, 2, 1, 2);
    scaleGrid->addWidget(fitTo, 1, 0);
    scaleGrid->addWidget(wide, 1, 1);
    scaleGrid->addWidget(new QLabel(QObject::tr("by")), 1, 2);
    scaleGrid->addWidget(tall, 1, 3);
    scaleGrid->addWidget(new QLabel(QObject::tr("pages")), 1, 4);

    auto *previewBox = new QGroupBox(QObject::tr("Preview"));
    (new QVBoxLayout(previewBox))->addWidget(preview);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto *grid = new QGridLayout(&dlg);
    grid->addWidget(previewBox, 0, 0, 2, 1);
    grid->addWidget(paperBox, 0, 1);
    grid->addWidget(orientBox, 1, 1);
    grid->addWidget(marginBox, 2, 0);
    grid->addWidget(centerBox, 2, 1);
    grid->addWidget(scaleBox, 3, 0, 1, 2);
    grid->addWidget(buttons, 4, 0, 1, 2);

    auto sync = [&] {
        work.layout.setPageSize(QPageSize(QPageSize::PageSizeId(sizeCombo->currentData().toInt())));
        work.layout.setOrientation(portrait->isChecked() ? QPageLayout::Portrait : QPageLayout::Landscape);
        work.layout.setUnits(unit);
        work.layout.setMinimumMargins(QMarginsF());
        work.layout.setMargins(QMarginsF(left->value(), top->value(), right->value(), bottom->value()));
        work.centerH = centerH->isChecked();
        work.centerV = centerV->isChecked();
        work.fit = fitTo->isChecked();
        work.percent = percent->value();
        work.fitWide = wide->value();
        work.fitTall = tall->value();
        percent->setEnabled(!work.fit);
        wide->setEnabled(work.fit);
        tall->setEnabled(work.fit);
        preview->update(work);
    };
    sync();
    for (QDoubleSpinBox *sp : {left, right, top, bottom}) QObject::connect(sp, &QDoubleSpinBox::valueChanged, &dlg, sync);
    for (QSpinBox *sp : {percent, wide, tall}) QObject::connect(sp, &QSpinBox::valueChanged, &dlg, sync);
    for (QAbstractButton *b : std::initializer_list<QAbstractButton *>{portrait, landscape, centerH, centerV, adjust, fitTo})
        QObject::connect(b, &QAbstractButton::toggled, &dlg, sync);
    QObject::connect(sizeCombo, &QComboBox::currentIndexChanged, &dlg, sync);

    if (dlg.exec() != QDialog::Accepted) return false;
    s = work;
    return true;
}

} // namespace printing
