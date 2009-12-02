# Copyright 1999-2009 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils subversion

DESCRIPTION="libpurple protocol plugin for Heroes of Newerth Chat Server"
HOMEPAGE=""

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="-*"
IUSE=""

RDEPEND=">=net-im/pidgin-2.6.0
	>=dev-libs/glib-2.16"
DEPEND="${RDEPEND}"

S=${WORKDIR}/honpurple

ESVN_REPO_URI="http://honpurple.googlecode.com/svn/trunk/"
ESVN_PROJECT="honpurple"

src_unpack() {
	subversion_src_unpack
}

src_compile() {
	emake || die "emake failed."
}

src_install () {
	emake install || die "emake failed"
}
