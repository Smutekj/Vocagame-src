#include "UIDocument.h"

#include <deque>

#include <Renderer.h>
#include <Font.h>


void UIElement::update() {}

void UIElement::drawBoundingBox(Renderer &canvas)
{
    utils::Vector2f ul = {bounding_box.pos_x, bounding_box.pos_y};
    utils::Vector2f ur = {bounding_box.pos_x + bounding_box.width, bounding_box.pos_y};
    utils::Vector2f ll = {bounding_box.pos_x, bounding_box.pos_y + bounding_box.height};
    utils::Vector2f lr = {bounding_box.pos_x + bounding_box.width, bounding_box.pos_y + bounding_box.height};

    canvas.drawLineBatched(ul, ur, 1, {0, 1, 0, 1});
    canvas.drawLineBatched(ul, ll, 1, {0, 1, 0, 1});
    canvas.drawLineBatched(ur, lr, 1, {0, 1, 0, 1});
    canvas.drawLineBatched(ll, lr, 1, {0, 1, 0, 1});

    //! padding box
    ul = {bounding_box.pos_x + padding.x, bounding_box.pos_y + padding.y};
    ur = {bounding_box.pos_x + bounding_box.width - padding.x, bounding_box.pos_y + padding.y};
    ll = {bounding_box.pos_x + padding.x, bounding_box.pos_y + bounding_box.height - padding.y};
    lr = {bounding_box.pos_x + bounding_box.width - padding.x, bounding_box.pos_y + bounding_box.height - padding.y};
    canvas.drawLineBatched(ul, ur, 1., {0, 1, 1, 1});
    canvas.drawLineBatched(ul, ll, 1., {0, 1, 1, 1});
    canvas.drawLineBatched(ur, lr, 1., {0, 1, 1, 1});
    canvas.drawLineBatched(ll, lr, 1., {0, 1, 1, 1});
}

void UIElement::calculateBoundingBoxSize()
{
    std::visit([this](auto &&w)
               {
        using T = std::decay_t<decltype(w)>;
        if constexpr (std::is_same_v<T, Pixels>)
        {
            bounding_box.width = w;
        }
        if constexpr (std::is_same_v<T, Percentage>)
        {
            bounding_box.width = (w * (float)parent->widthContent());
        }
        if constexpr (std::is_same_v<T, Viewport>)
        {
            // assert(false && "Don't do this, you were too lazy to finish it!");
        } }, dimensions.x);

    std::visit([this](auto &&h)
               {
        using T = std::decay_t<decltype(h)>;
        if constexpr (std::is_same_v<T, Pixels>)
        {
            bounding_box.height = h;
        }
        if constexpr (std::is_same_v<T, Percentage>)
        {
            bounding_box.height = h * (float)parent->heightContent();
        }
        if constexpr (std::is_same_v<T, Viewport>)
        {
            // assert(false && "Don't do this, you were too lazy to finish it!");
        } }, dimensions.y);
}

void UIElement::draw(Renderer &canvas)
{
//    drawBoundingBox(canvas);
};

double UIElement::maxChildrenHeight() const
{
    if (children.empty())
    {
        return 0.;
    }
    auto largest_child_p = *std::max_element(children.begin(), children.end(), [](auto &c1, auto &c2)
                                             { return c1->bounding_box.height + 2 * c1->margin.y < c2->bounding_box.height + 2 * c2->margin.y; });
    return largest_child_p->bounding_box.height + 2 * largest_child_p->margin.y;
}
double UIElement::maxChildrenWidth() const
{
    if (children.empty())
    {
        return 0.;
    }
    auto largest_child_p = *std::max_element(children.begin(), children.end(), [](auto &c1, auto &c2)
                                             { return c1->bounding_box.width + 2 * c1->margin.x < c2->bounding_box.width + 2 * c2->margin.x; });
    return largest_child_p->bounding_box.width + 2 * largest_child_p->margin.x;
}
double UIElement::totalChildrenWidth() const
{
    return std::accumulate(children.begin(), children.end(), 0., [](double val, auto &c_p)
                           { return val + c_p->bounding_box.width; });
}
double UIElement::totalChildrenHeight() const
{
    return std::accumulate(children.begin(), children.end(), 0., [](double val, auto &c_p)
                           { return val + c_p->bounding_box.height; });
}

void UIElement::addChild(UIElementP child_el)
{
    child_el->parent = this;
    if (children.size() > 0)
    {
        children.back()->next_sibling = child_el.get();
        child_el->prev_sibling = children.back().get();
    }
    children.push_back(child_el);
}
void UIElement::addChildAfter(UIElementP child_el, UIElementP after_who)
{
    auto it = std::find(children.begin(), children.end(), after_who);
    if (it != children.end())
    {
        child_el->prev_sibling = after_who.get();
        child_el->next_sibling = after_who->next_sibling;
        after_who->next_sibling = child_el.get();
        if (after_who->next_sibling)
        {
            after_who->next_sibling->prev_sibling = child_el.get();
        }
    }
    else
    {
        addChild(child_el);
    }
}

utils::Vector2i UIElement::totalChildrenMargin() const
{
    return std::accumulate(children.begin(), children.end(), utils::Vector2i{0, 0}, [](auto val, auto &c_p)
                           {
                               return val + 2 * c_p->margin; //! margin is on both sides so x2
                           });
}

void UIElement::drawX(Renderer &canvas)
{
    int max_x = 0;

    if (parent && bounding_box.width == 0)
    {
        max_width = parent->width();
    }
    else if (!parent && max_width == 0)
    {
        max_width = canvas.getTargetSize().x;
    }
    else
    {
        max_width = bounding_box.width;
    }
    max_x = max_width + bounding_box.pos_x - padding.x;

    //! resize ourselves or children depending on sizing mode
    double total_content_width = totalChildrenWidth();
    double total_content_height = totalChildrenHeight();
    auto total_margin = totalChildrenMargin();

    if (sizing == Sizing::FIXED)
    {
        calculateBoundingBoxSize();
    }
    if (sizing == Sizing::RESCALE_CHILDREN)
    {
        double scale = 1;
        for (auto &child : children)
        {
            if (layout == Layout::X)
            {
                scale = (bounding_box.width - 2 * padding.x - total_margin.x) / (total_content_width);
            }
            else if (layout == Layout::Y)
            {
                scale = (bounding_box.height - 2 * padding.y - total_margin.y) / (total_content_height);
            }
            child->bounding_box.width *= scale;
            child->bounding_box.height *= scale;
        }
    }
    if (sizing == Sizing::SCALE_TO_FIT)
    {
        if (layout == Layout::X)
        {
            bounding_box.width = total_content_width + total_margin.x + 2 * padding.x;
            bounding_box.height = maxChildrenHeight() + 2 * padding.y;
        }
        else if (layout == Layout::Y)
        {
            bounding_box.width = maxChildrenWidth() + 2 * padding.x;
            bounding_box.height = total_content_height + total_margin.y + 2 * padding.y;
        }
    }

    // ! align myself within my bb
    if (parent)
    {
        if (align_x == Alignement::Right)
        {
            bounding_box.pos_x = (parent->rightContent() - width() - margin.x);
        }
        if (align_x == Alignement::Center || align_x == Alignement::CenterX)
        {
            bounding_box.pos_x = (parent->centerContent().x - width() / 2);
        }
        if (align_x == Alignement::Left)
        {
            bounding_box.pos_x = (parent->leftContent() + margin.x);
        }
        if (align_y == Alignement::Center || align_y == Alignement::CenterY)
        {
            bounding_box.pos_y = (parent->centerContent().y - height() / 2);
        }
        if (align_y == Alignement::Top)
        {
            bounding_box.pos_y = (parent->topContent() + margin.y);
        }
    }

    int x = leftContent();
    int y = topContent();

    int prev_height = 0;
    //! set children box positions
    int children_width = 0;
    int children_height = 0;
    if (layout == Layout::Grid)
    {
        int max_row_height = 0;
        for (int i = 0; i < children.size(); ++i)
        {
            auto &child = *children.at(i);
            auto &child_box = children.at(i)->bounding_box;

            if (child_box.width + x > max_x) //! start next row
            {
                x = bounding_box.pos_x + padding.x;
                y += max_row_height + margin.y;
            }
            child_box.pos_x = x + child.margin.x;
            child_box.pos_y = y + child.margin.y;

            x += (2 * children.at(i)->margin.x + child_box.width);
            max_row_height = std::max(child.height() + child.margin.y, max_row_height);
        }
    }
    else if (layout == Layout::X)
    {
        int total_content_width = totalChildrenWidth() + totalChildrenMargin().x;
        //! place the first child based on allignement
        if (content_align_x == Alignement::Right)
        {
            x = rightContent() - total_content_width;
        }
        else if (content_align_x == Alignement::Center || content_align_x == Alignement::CenterX)
        {
            x = centerContent().x - total_content_width / 2.;
        }
        if (content_align_y == Alignement::Center || content_align == Alignement::CenterY)
        {
            y = centerContent().y - maxChildrenHeight() / 2.;
        }
        else if (content_align_y == Alignement::Bottom)
        {
            y = topContent() + (heightContent() - maxChildrenHeight());
        }

        for (int i = 0; i < children.size(); ++i)
        {
            auto &child = *children.at(i);

            child.bounding_box.pos_x = x + child.margin.x;
            child.bounding_box.pos_y = y + child.margin.y;

            x += (2 * children.at(i)->margin.x + child.width());
        }
    }
    else if (layout == Layout::Y)
    {
        int total_content_height = totalChildrenHeight() + totalChildrenMargin().y;
        //! place the first child based on allignement
        if (content_align_x == Alignement::Right)
        {
            x = rightContent() - maxChildrenWidth();
        }
        else if (content_align_x == Alignement::Center || content_align_x == Alignement::CenterX)
        {
            x = centerContent().x - maxChildrenWidth() / 2.;
        }
        if (content_align_y == Alignement::Center || content_align_y == Alignement::CenterY)
        {
            y = centerContent().y - total_content_height / 2.;
        }
        else if (content_align_y == Alignement::Bottom)
        {
            y = topContent() + (heightContent() - total_content_height);
        }

        for (int i = 0; i < children.size(); ++i)
        {
            auto &child = *children.at(i);

            child.bounding_box.pos_x = x + child.margin.x;
            child.bounding_box.pos_y = y + child.margin.y;

            y += (2 * children.at(i)->margin.y + child.height());
        }
    }

    draw(canvas);
    canvas.drawAll();

    for (auto &child : children)
    {
        child->drawX(canvas);
    }
}

int UIElement::width() const
{
    return bounding_box.width;
}
int UIElement::height() const
{
    return bounding_box.height;
}
int UIElement::x() const
{
    return bounding_box.pos_x;
}
int UIElement::y() const
{
    return bounding_box.pos_y;
}

int UIElement::widthContent() const
{
    return bounding_box.width - padding.x * 2;
}
int UIElement::heightContent() const
{
    return bounding_box.height - padding.y * 2;
}

int UIElement::rightContent() const
{
    return bounding_box.pos_x + bounding_box.width - padding.x;
}
int UIElement::right() const
{
    return bounding_box.pos_x + bounding_box.width;
}
int UIElement::leftContent() const
{
    return bounding_box.pos_x + padding.x;
}
int UIElement::left() const
{
    return bounding_box.pos_x;
}
int UIElement::topContent() const
{
    return bounding_box.pos_y + padding.y;
}
int UIElement::top() const
{
    return bounding_box.pos_y;
}
utils::Vector2i UIElement::centerContent() const
{
    return {bounding_box.pos_x + padding.x + (bounding_box.width - 2 * padding.x) / 2.,
            bounding_box.pos_y + padding.y + (bounding_box.height - 2 * padding.y) / 2.};
}
int UIElement::center() const
{
    return bounding_box.pos_x + bounding_box.width / 2;
}

UIDocument::UIDocument(Renderer &window_canvas)
    : document(window_canvas)
{

    // XMLDocument doc;
    // doc.LoadFile( "../../ui.xml" );

    // XMLElement* xml_elem = doc.RootElement();
    // std::vector<XMLElement*> to_visit;
    // to_visit.push_back(xml_elem);
    // std::vector<XMLElement*> to_backtrack;
    // while(!to_visit.empty())
    // {
    //     xml_elem = to_visit.back();
    //     to_visit.pop_back();

    //     to_backtrack.push_back(xml_elem);
    //     for (XMLElement* child = xml_elem->FirstChildElement(); child; child = child->NextSiblingElement())
    //     {
    //         to_visit.push_back(child);
    //     }
    // }

    // for(int i = to_backtrack.size() - 1; i >= 0; i--)
    // {
    //     if(to_backtrack.at(i)->Name() == "SpriteUIElement")
    //     {

    //     }else if (to_backtrack.at(i)->Name() == "TextUIElement")
    //     {

    //     }else if (to_backtrack.at(i)->Name() == "UIElement")
    //     {

    //     }else if (to_backtrack.at(i)->Name() == "SpriteUIElement")
    //     {

    //     }
    // }

    root = std::make_shared<UIElement>();
    root->parent = nullptr;
    root->bounding_box = {0, 0,
                          window_canvas.getTargetSize().x,
                          window_canvas.getTargetSize().y};
}

void UIDocument::onEvent(UIEvent event, UIElement::UIElementP node_p)
{
    if (node_p->event_callbacks.count(event) != 0)
    {
        node_p->event_callbacks.at(event)(node_p);
    }
}

void UIDocument::handleEvent(UIEvent event)
{
    if (event == UIEvent::MOUSE_ENETERED)
    {
        forEach([event, this](auto node_p)
                {
                bool mouse_is_inside = node_p->bounding_box.contains(document.getMouseInScreen());
                if (!node_p->mouse_hovering && mouse_is_inside)
                {
                    node_p->mouse_hovering = true;
                    onEvent(UIEvent::MOUSE_ENETERED, node_p);
                }
                if (node_p->mouse_hovering && !mouse_is_inside)
                {
                    node_p->mouse_hovering = false;
                    onEvent(UIEvent::MOUSE_LEFT, node_p);
                
                } });
        return;
    }

    forEach([event, this](auto node_p)
            {
                bool mouse_is_inside = node_p->bounding_box.contains(document.getMouseInScreen());
                if(mouse_is_inside)
                {
                    onEvent(UIEvent::CLICK, node_p);
                } });
}

UIElement *UIElement::getElementById(const std::string &id)
{
    std::deque<UIElement *> to_visit;
    to_visit.push_back(this);

    while (!to_visit.empty())
    {
        auto curr = to_visit.front();
        to_visit.pop_front();
        if (curr->id == id)
        {
            return curr;
        }
        for (auto &child : curr->children)
        {
            to_visit.push_back(child.get());
        }
    }
    return nullptr;
}

bool UIElement::removeElementById(const std::string &id)
{
    std::deque<std::pair<int, UIElement *>> to_visit;
    to_visit.push_back({0, this});

    while (!to_visit.empty())
    {
        auto curr = to_visit.front().second;
        int child_id = to_visit.front().first;
        to_visit.pop_front();

        int i = 0;
        for (auto &child : curr->children)
        {
            if (child->id == id)
            {
                curr->children.erase(curr->children.begin() + i);
                return true;
            }
            to_visit.push_back({i, child.get()});
            i++;
        }
    }
    //! element with id does not exist
    return false;
}

UIElement *UIDocument::getElementById(const std::string &id) const
{
    return root->getElementById(id);
}
bool UIDocument::removeElementById(const std::string &id) const
{
    return root->removeElementById(id);
}

void UIDocument::forEach(std::function<void(UIElement::UIElementP)> callback)
{
    std::deque<UIElement::UIElementP> to_visit;
    to_visit.push_back(root);
    while (!to_visit.empty())
    {
        auto curr = to_visit.front();
        to_visit.pop_front();

        callback(curr);

        for (auto &child : curr->children)
        {
            to_visit.push_back(child);
        }
    }
}

void UIDocument::drawBoundingBoxes()
{
    forEach([this](auto el_p)
            { el_p->drawBoundingBox(document); });
}

void UIDocument::drawUI()
{
    root->drawX(document);
}

MultiLineUIElement::MultiLineUIElement(Font &font, std::string text)
{
    m_text.setFont(&font);
    m_text.setText(text);
}

void MultiLineUIElement::draw(Renderer &canvas)
{
    m_text.setPosition({bounding_box.pos_x, bounding_box.pos_y});
    m_text.setPadding(padding);
    m_text.setPageWidth(bounding_box.width);

    m_text.drawInto(canvas);
}

TextUIELement::TextUIELement(Font &font, std::string text)
    : m_text(text)
{
    setFont(font);
}

void TextUIELement::draw(Renderer &canvas)
{
    UIElement::draw(canvas);

    utils::Vector2f center_pos = {bounding_box.pos_x + width() / 2.,
                                  bounding_box.pos_y + height() / 2.};

    utils::Vector2f size = {width(), height()};
    auto text_bounder = m_text.getBoundingBox();
    utils::Vector2f scale = {width(), height()};

    float error_x = text_bounder.width - (size.x - 2 * padding.x);
    float error_y = text_bounder.height - (size.y - 2 * padding.y);

    float sx = std::abs((size.x - 2 * padding.x) / (text_bounder.width));
    float sy = std::abs((size.y - 2 * padding.y) / (text_bounder.height));

    if (error_x > 0)
    {
        m_text.scale(sx, -sx);
    }
    else if (error_y > 0)
    {
        m_text.scale(sy, -sy);
    }
    else
    {
        if (m_text.getScale().y > 0.)
        {
            m_text.scale(1., -1);
        }
    }

    m_text.setPosition(center_pos.x, center_pos.y - text_bounder.height / 2);
    m_text.centerAround({center_pos.x, center_pos.y});
    canvas.drawText(m_text);
}

void TextUIELement::setFont(Font &font)
{
    m_text.setFont(&font);
}

void SpriteUIELement::draw(Renderer &canvas)
{
    UIElement::draw(canvas);

    utils::Vector2f center_pos = {bounding_box.pos_x + width() / 2.,
                                  bounding_box.pos_y + height() / 2.};

    utils::Vector2f size = {width(), -height()};
    image.setScale(size / 2.f);
    image.setPosition(center_pos);
    if (m_shader_id.empty())
    {
        canvas.drawSprite(image);
    }
    else
    {
        canvas.drawSprite(image, m_shader_id);
    }
}
void SpriteUIELement::setTexture(Texture &tex)
{
    image.setTexture(tex);
}
