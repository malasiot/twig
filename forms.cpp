#include <twig/form_helper.hpp>
#include <twig/validators.hpp>
#include <twig/renderer.hpp>
#include <iostream>

using namespace twig ;
using namespace std ;

int main(int argc, char *argv[]) {
    Form form("/request", "POST") ;

    form.field<TextField>("username")
        .label("Username")
        .attribute("required", true)
        .attribute("placeholder", "Write user name")
        .attribute("class", Variant::Array{"grid", "flex"})
        .addValidator(FormFieldValidators::required())
        .addValidator(FormFieldValidators::matches(std::regex("[\\._]+"))) ;

    form.field<SelectField>("role")
        .addOption("admin", "Admin")
        .addOption("guest", "Guest")
        .label("Role");

    form.process({{"username", "john"}});

   cout << Variant(form.render()).toJSON() << endl ;
      std::string twig_template = R"(
      <form action="{{ form.action }}" method="{{ form.method }}">
        {% for k,field in form.fields %}
        <div class="form-group {% if field.errors is not empty %}has-error{% endif %}">
        
        {%if field.widget == 'select' %}
        <label>{{ field.label }}</label>
            <select name="{{ field.name }}" value="{{ field.value }}">
             {% for option in field.options %}
                    <option value="{{ option.value }}" {% if option.selected %}selected{% endif %}>{{ option.label }}</option>
                {% endfor %}
            </select>
              {% elif field.widget == "radio" %}
            <span class="group-label"><strong>{{ field.label }}</strong></span>
            <div class="radio-options-wrapper">
                {% for option in field.options %}
                    <label class="radio-inline">
                        <input type="radio" 
                               name="{{ field.name }}" 
                               value="{{ option.value }}"
                               {{ html_attr(option.attr) }}>
                        {{ option.label }}
                    </label>
                {% endfor %}
            </div>

        {# --- 2. SELECT DROP-DOWNS WIDGET --- #}
            {%elif field.widget == 'input'%}
                {% if field.type == "checkbox" %}
                <label>
                    <input type="checkbox" name="{{ field.name }}" {% if field.checked %}checked{% endif %} {{html_attr(field.attr)}}>
                    {{ field.label }}
                </label>
            {% else %}
                <label>{{ field.label }}</label>
                <input type="{{ field.type }}" name="{{ field.name }}" value="{{ field.value }}" {{html_attr(field.attr)}}>
            {% endif %}

            {%endif%}
       
        {% for error in field.errors %}
            <span class="err">{{ error }}</span>
        {% endfor %}
    </div>
    {% endfor %}
    <button type="submit">Register</button>
</form>
    )";

    TemplateRenderer rdr(nullptr) ;
    cout << rdr.renderString(twig_template, {{"form", form.render()}}) << endl;
}