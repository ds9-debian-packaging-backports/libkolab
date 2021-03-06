/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mimeutils.h"
#include <quuid.h>
#include <QtCore/qfile.h>
#include <qdom.h>
#include <kdatetime.h>
#include "kolabformat/kolabdefinitions.h"
#include "kolabformat/errorhandler.h"
#include <kolabformat.h>
#include "libkolab-version.h"

namespace Kolab {
    namespace Mime {

KMime::Content* findContentByType(const KMime::Message::Ptr &data, const QByteArray &type)
{
    if (type.isEmpty()) {
        Error() << "Empty type";
        return 0;
    }
    Q_ASSERT(!data->contents().isEmpty());
    Q_FOREACH(KMime::Content *c, data->contents()) {
//         qDebug() << c->contentType()->mimeType() << type;
        if (c->contentType()->mimeType() ==  type) {
            return c;
        }
    }
    return 0;
}

KMime::Content* findContentByName(const KMime::Message::Ptr &data, const QString &name, QByteArray &type)
{
    Q_ASSERT(!data->contents().isEmpty());
    Q_FOREACH(KMime::Content *c, data->contents()) {
//         qDebug() << "searching: " << c->contentType()->name().toUtf8();
        if ( c->contentType()->name() == name ) {
            type = c->contentType()->mimeType();
            return c;
        }
    }
    return 0;
}

KMime::Content* findContentById(const KMime::Message::Ptr &data, const QByteArray &id, QByteArray &type, QString &name)
{
    if (id.isEmpty()) {
        Error() << "looking for empty cid";
        return 0;
    }
    Q_ASSERT(!data->contents().isEmpty());
    Q_FOREACH(KMime::Content *c, data->contents()) {
//         qDebug() << "searching: " << c->contentID()->identifier();
        if ( c->contentID()->identifier() == id ) {
            type = c->contentType()->mimeType();
            name = c->contentType()->name();
            return c;
        }
    }
    return 0;
}

QList<QByteArray> getContentMimeTypeList(const KMime::Message::Ptr& data)
{
    QList<QByteArray> typeList;
    Q_ASSERT(!data->contents().isEmpty());
    Q_FOREACH(KMime::Content *c, data->contents()) {
        typeList.append(c->contentType()->mimeType());
    }
    return typeList;
}

QString fromCid(const QString &cid)
{
    if (cid.left(4) != QString::fromLatin1("cid:")) { //Don't set if not a cid, happens when serializing format v2
        return QString();
    }
    return cid.right(cid.size()-4);
}

KMime::Message::Ptr createMessage(const QByteArray &mimetype, const QByteArray &xKolabType, const QByteArray &xml, bool v3, const QByteArray &productId,
                                  const QByteArray &fromEmail, const QString &fromName, const QString &subject)
{
    KMime::Message::Ptr message = createMessage(xKolabType, v3, productId);
    message->subject()->fromUnicodeString(subject, "utf-8");
    if (!fromEmail.isEmpty()) {
        KMime::Types::Mailbox mb;
        mb.setName(fromName);
        mb.setAddress(fromEmail);
        message->from()->addAddress(mb);
    }
    message->addContent(createMainPart(mimetype, xml));
    return message;
}

KMime::Message::Ptr createMessage(const std::string &mimetype, const std::string &xKolabType, const std::string &xml, bool v3, const std::string &productId,
                                  const std::string &fromEmail, const std::string &fromName, const std::string &subject)
{
    return createMessage(QByteArray(mimetype.c_str()), QByteArray(xKolabType.c_str()), QByteArray(xml.c_str()), v3, QByteArray(productId.data()), QByteArray(fromEmail.c_str()), QString::fromStdString(fromName), QString::fromStdString(subject));
}

KMime::Message::Ptr createMessage(const QString &subject, const QString &mimetype, const QString &xKolabType, const QByteArray &xml, bool v3, const QString &prodid)
{
    KMime::Message::Ptr message = createMessage( xKolabType.toLatin1(), v3, prodid.toLatin1() );
    if (!subject.isEmpty()) {
        message->subject()->fromUnicodeString( subject, "utf-8" );
    }
    
    KMime::Content *content = createMainPart( mimetype.toLatin1(), xml );
    message->addContent( content );
    
    message->assemble();
    return message;
}

KMime::Content* createExplanationPart(bool v3)
{
    KMime::Content *content = new KMime::Content();
    content->contentType()->setMimeType( "text/plain" );
    content->contentType()->setCharset( "us-ascii" );
    content->contentTransferEncoding()->setEncoding( KMime::Headers::CE7Bit );
    if (v3) {
        content->setBody( "This is a Kolab Groupware object.\n"
        "To view this object you will need an email client that can understand the Kolab Groupware format.\n"
        "For a list of such email clients please visit\n"
        "http://www.kolab.org/get-kolab\n" );
    } else {
        content->setBody( "This is a Kolab Groupware object.\n"
        "To view this object you will need an email client that can understand the Kolab Groupware format.\n"
        "For a list of such email clients please visit\n"
        "http://www.kolab.org/get-kolab\n" );
    }
    return content;
}


KMime::Message::Ptr createMessage(const QByteArray& xKolabType, bool v3, const QByteArray &prodid)
{
    KMime::Message::Ptr message(new KMime::Message);
    message->date()->setDateTime(KDateTime::currentUtcDateTime().dateTime());
    KMime::Headers::Generic* h = new KMime::Headers::Generic(X_KOLAB_TYPE_HEADER);
    h->fromUnicodeString(xKolabType, "utf-8");
    message->appendHeader(h);
    if (v3) {
        KMime::Headers::Generic* hv3 = new KMime::Headers::Generic(X_KOLAB_MIME_VERSION_HEADER);
        hv3->fromUnicodeString(KOLAB_VERSION_V3, "utf-8");
        message->appendHeader(hv3);
    }
    message->userAgent()->from7BitString(prodid);
    message->contentType()->setMimeType("multipart/mixed");
    message->contentType()->setBoundary(KMime::multiPartBoundary());
    message->addContent(createExplanationPart(v3));
    return message;
}


KMime::Content* createMainPart(const QByteArray& mimeType, const QByteArray& decodedContent)
{
    KMime::Content* content = new KMime::Content();
    content->contentType()->setMimeType(mimeType);
    content->contentType()->setName(KOLAB_OBJECT_FILENAME, "us-ascii");
    content->contentTransferEncoding()->setEncoding( KMime::Headers::CEquPr );
    content->contentDisposition()->setDisposition( KMime::Headers::CDattachment );
    content->contentDisposition()->setFilename( KOLAB_OBJECT_FILENAME );
    content->setBody(decodedContent);
    return content;
}

KMime::Content* createAttachmentPart(const QByteArray& cid, const QByteArray& mimeType, const QString& fileName, const QByteArray& base64EncodedContent)
{
    KMime::Content* content = new KMime::Content();
    if (!cid.isEmpty()) {
        content->contentID()->setIdentifier( cid );
    }
    content->contentType()->setMimeType( mimeType );
    content->contentType()->setName( fileName, "utf-8" );
    content->contentTransferEncoding()->setEncoding( KMime::Headers::CEbase64 );
    content->contentDisposition()->setDisposition( KMime::Headers::CDattachment );
    content->contentDisposition()->setFilename( fileName );
    content->setBody( base64EncodedContent );
    return content;
}

Kolab::Attachment getAttachment(const std::string &id, const KMime::Message::Ptr &mimeData)
{
    if (!QString::fromStdString(id).contains("cid:")) {
        Error() << "not a cid reference";
        return Kolab::Attachment();
    }
    QByteArray type;
    QString name;
    KMime::Content *content = findContentById(mimeData, fromCid(QString::fromStdString(id)).toLatin1(), type, name);
    if (!content) { // guard against malformed events with non-existent attachments
        Error() << "could not find attachment: "<< name << type;
        return Kolab::Attachment();
    }
    // Debug() << id << content->decodedContent().toBase64().toStdString();
    Kolab::Attachment attachment;
    attachment.setData(content->decodedContent().toStdString(), type.toStdString());
    attachment.setLabel(name.toStdString());
    return attachment;
}

Kolab::Attachment getAttachmentByName(const QString &name, const KMime::Message::Ptr &mimeData)
{
    QByteArray type;
    KMime::Content *content = findContentByName(mimeData, name, type);
    if (!content) { // guard against malformed events with non-existent attachments
        Warning() << "could not find attachment: "<< name.toUtf8() << type;
        return Kolab::Attachment();
    }
    Kolab::Attachment attachment;
    attachment.setData(content->decodedContent().toStdString(), type.toStdString());
    attachment.setLabel(name.toStdString());
    return attachment;
}


}; //Namespace
}; //Namespace
