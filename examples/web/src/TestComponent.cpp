/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "SPWebHostComponent.h"
#include "SPWebRequestHandler.h"
#include "SPValid.h"

namespace stappler::web::test {

class TestComponent : public HostComponent {
public:
	TestComponent(const Host &serv, const HostComponentInfo &info);
	virtual ~TestComponent() { }

	virtual void handleChildInit(const Host &) override;
	virtual void initTransaction(db::Transaction &) override;

protected:
	Scheme _objects = Scheme("objects");
};

TestComponent::TestComponent(const Host &serv, const HostComponentInfo &info)
: HostComponent(serv, info) {
	// Экспортируем схему данных на сервер
	exportValues(_objects);

	using namespace db;

	// Определяем простую схему данных
	_objects.define(Vector<Field>{
		Field::Text("text", MinLength(3)),
		Field::Extra("data", Vector<Field>{
			Field::Array("strings", Field::Text("")),
		}),
		Field::File("image", MaxFileSize(1_MiB)),
		Field::Text("alias", Transform::Alias),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
	},
	AccessRole::Admin(AccessRoleId::Authorized));
}

void TestComponent::handleChildInit(const Host &serv) {
	// Создаём автоматический ресурс для схемы данных
	serv.addResourceHandler("/objects/", _objects);

	// Пример административной команды, экспортируемой компонентом
	addOutputCommand("test", [&] (StringView str, const Callback<void(const Value &)> &cb) -> bool {
		if (auto t = db::Transaction::acquireIfExists()) {
			// cb вызывается для вывода в консоль
			cb(Value("test"));
		}
		return true;
	}, " - test");
}

void TestComponent::initTransaction(db::Transaction &t) {
	// Принудительно устанавливаем роль пользователя
	t.setRole(db::AccessRoleId::Authorized);
}

// Функция загрузки компонента
SP_EXTERN_C HostComponent * CreateTestComponent(const Host &serv, const HostComponentInfo &info) {
	return new TestComponent(serv, info);
}

}
