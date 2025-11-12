//
// Created by RGAA on 2024/4/8.
//

#include "qr_generator.h"
#include "qrcodegen.hpp"
#include "tc_common_new/log.h"

using namespace qrcodegen;

namespace tc
{

    QPixmap QrGenerator::GenQRPixmap(const QString &message, int qr_size) {
        std::vector<QrSegment> segs = QrSegment::makeSegments(message.toStdString().c_str());
        // QrCode qr1 = QrCode::encodeSegments(segs, QrCode::Ecc::QUARTILE, 15, 15, 2, false);
        QrCode qr1 = QrCode::encodeSegments(segs, QrCode::Ecc::LOW, 5, 15, 1, false);
        QImage QrCode_Image=QImage(qr1.getSize(),qr1.getSize(),QImage::Format_RGB888);
        QrCode_Image.fill(Qt::transparent);
        LOGI("QRCode, size: {}", qr1.getSize());
        for (int y = 0; y < qr1.getSize(); y++) {
            for (int x = 0; x < qr1.getSize(); x++) {
                if(qr1.getModule(x, y)) {
                    QrCode_Image.setPixel(x, y, qRgb(0, 0, 0));
                    //QrCode_Image.setPixel(x, y, qRgb(200-x*5+y*x, 100+x*y, x+10*y-2));
                } else {
                    QrCode_Image.setPixel(x, y, qRgb(255, 255, 255));
                }
            }
        }
        if (qr_size != -1) {
            QrCode_Image = QrCode_Image.scaled(qr_size, qr_size, Qt::KeepAspectRatio);
        }
        return QPixmap::fromImage(QrCode_Image);
    }

}
