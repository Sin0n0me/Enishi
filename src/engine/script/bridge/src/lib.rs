use musubu_ast::{ASTNode, Item};
use musubu_cache::Allocator;
use musubu_engine::MusubuEngine;
use musubu_primitive::{Float, Integer, PrimitiveType, ToPrimitiveType, Value};
use std::panic::{AssertUnwindSafe, catch_unwind};

// Must match sub_system::ScriptValueKind in the host ABI.
#[derive(Clone, Copy, Default)]
#[repr(u32)]
enum Kind {
    #[default]
    Unit,
    Boolean,
    SignedInteger,
    UnsignedInteger,
    Float,
    String,
    Tuple,
    Array,
    Vector,
    Matrix,
    Struct,
    Enum,
    Range,
}

#[derive(Default)]
pub struct Node {
    kind: Kind,
    signed: i64,
    unsigned: u64,
    number: f64,
    text: String,
    type_name: String,
    name: String,
    children: Vec<Node>,
}

#[repr(C)]
pub struct View {
    kind: u32,
    signed: i64,
    unsigned: u64,
    number: f64,
    text: *const u8,
    text_len: usize,
    type_name: *const u8,
    type_len: usize,
    name: *const u8,
    name_len: usize,
    child_count: usize,
}

pub struct Execution {
    error: String,
    value: Node,
}

fn integer(value: &Integer, node: &mut Node) {
    use Integer::*;
    match value {
        Int8(v) => node.signed = i64::from(*v),
        Int16(v) => node.signed = i64::from(*v),
        Int32(v) => node.signed = i64::from(*v),
        Int64(v) => node.signed = *v,
        Uint8(v) => node.unsigned = u64::from(*v),
        Uint16(v) => node.unsigned = u64::from(*v),
        Uint32(v) => node.unsigned = u64::from(*v),
        Uint64(v) => node.unsigned = *v,
    }
    node.kind = if matches!(value, Int8(_) | Int16(_) | Int32(_) | Int64(_)) {
        Kind::SignedInteger
    } else {
        Kind::UnsignedInteger
    };
}

fn children(values: &[Value]) -> Result<Vec<Node>, String> {
    values.iter().map(convert).collect()
}

fn convert(value: &Value) -> Result<Node, String> {
    let mut node = Node::default();
    node.type_name = match value {
        Value::String(_) => "string".into(),
        Value::Pointer => {
            return Err("Pointer results cannot be transferred to host systems".into());
        }
        _ => value.to_type().to_string(),
    };
    match value {
        Value::Unit => {}
        Value::Bool(v) => {
            node.kind = Kind::Boolean;
            node.unsigned = u64::from(*v);
        }
        Value::Integer(v) => integer(v, &mut node),
        Value::Float(v) => {
            node.kind = Kind::Float;
            node.number = match v {
                Float::Float32(v) => f64::from(*v),
                Float::Float64(v) => *v,
            };
        }
        Value::String(v) => {
            node.kind = Kind::String;
            node.text = v.clone();
        }
        Value::Tuple { fields, .. } => {
            node.kind = Kind::Tuple;
            node.children = children(fields)?;
        }
        Value::Array { elements, .. } => {
            node.kind = Kind::Array;
            node.children = children(elements)?;
        }
        Value::Vector(v) => {
            node.kind = Kind::Vector;
            node.children = children(&v.components())?;
        }
        // Matrix children are columns, each containing row components.
        Value::Matrix(v) => {
            node.kind = Kind::Matrix;
            node.children = children(&v.columns())?;
        }
        Value::Struct {
            fields,
            struct_type,
        } => {
            node.kind = Kind::Struct;
            node.children = children(fields)?;
            if let PrimitiveType::NamedStruct { name, fields } = struct_type {
                node.type_name = name.clone();
                for (child, (name, _)) in node.children.iter_mut().zip(fields) {
                    child.name = name.clone();
                }
            }
        }
        Value::Enum {
            variant,
            fields,
            enum_type,
        } => {
            node.kind = Kind::Enum;
            node.unsigned = *variant as u64;
            node.children = children(fields)?;
            if let PrimitiveType::NamedEnum { name, variants } = enum_type {
                node.type_name = name.clone();
                if let Some(v) = variants.get(*variant) {
                    node.text = v.name.clone();
                    for (child, (name, _)) in node.children.iter_mut().zip(&v.fields) {
                        child.name = name.clone();
                    }
                }
            }
        }
        Value::Range {
            start,
            end,
            inclusive,
        } => {
            node.kind = Kind::Range;
            node.unsigned = u64::from(*inclusive);
            node.children =
                children(&[Value::Integer(start.clone()), Value::Integer(end.clone())])?;
        }
        Value::Pointer => unreachable!(),
    }
    Ok(node)
}

fn execute(source: &str) -> Result<Node, String> {
    // A fresh engine prevents stale functions from a previous successful run surviving errors.
    let mut engine = MusubuEngine::new();
    if !musubu_driver::compile_with_filename(&mut engine, "script", source) {
        return Err(engine.compile_error().to_owned());
    }
    let tokens = musubu_lexer::tokenize(source).map_err(|e| format!("{e:?}"))?;
    let ast = musubu_parser::parse(&tokens).map_err(|e| format!("{e:?}"))?;
    let main = ast.iter().find_map(|node| match node.as_ref() {
        ASTNode::Item { item, .. } => match &item.node {
            Item::Function {
                name, params, body, ..
            } if name == "main" => Some((params, body)),
            _ => None,
        },
        _ => None,
    });
    match main {
        Some((params, body)) if params.is_empty() && body.is_some() => {}
        _ => return Err("Define an argument-free function: fn main() { ... }".into()),
    }
    // Musubu's resolver allocates by module-qualified name, not source declaration order.
    let entry = engine
        .get_cache()
        .get_function_allocator()
        .alloc("musubu::main".into());
    let value = engine
        .run_function(entry, vec![])
        .map_err(|e| format!("Runtime error: {e:?}"))?;
    convert(&value.unwrap_or(Value::Unit))
}

// All pointers returned below are borrowed from Execution until enishi_musubu_free.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn enishi_musubu_execute(source: *const u8, length: usize) -> *mut Execution {
    let outcome = catch_unwind(AssertUnwindSafe(|| {
        if source.is_null() {
            return Err("Source is null".into());
        }
        let bytes = unsafe { std::slice::from_raw_parts(source, length) };
        let source = std::str::from_utf8(bytes).map_err(|e| e.to_string())?;
        execute(source)
    }))
    .unwrap_or_else(|panic| {
        let message = panic
            .downcast_ref::<String>()
            .map(String::as_str)
            .or_else(|| panic.downcast_ref::<&str>().copied())
            .unwrap_or("unknown panic");
        Err(format!("Musubu execution failed: {message}"))
    });
    Box::into_raw(Box::new(match outcome {
        Ok(value) => Execution {
            error: String::new(),
            value,
        },
        Err(error) => Execution {
            error,
            value: Node::default(),
        },
    }))
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn enishi_musubu_free(result: *mut Execution) {
    if !result.is_null() {
        drop(unsafe { Box::from_raw(result) });
    }
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn enishi_musubu_error(
    result: *const Execution,
    length: *mut usize,
) -> *const u8 {
    let result = unsafe { &*result };
    unsafe {
        *length = result.error.len();
    }
    result.error.as_ptr()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn enishi_musubu_value(result: *const Execution) -> *const Node {
    &unsafe { &*result }.value
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn enishi_musubu_child(node: *const Node, index: usize) -> *const Node {
    unsafe { &*node }
        .children
        .get(index)
        .map_or(std::ptr::null(), |n| n)
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn enishi_musubu_view(node: *const Node, view: *mut View) {
    let n = unsafe { &*node };
    unsafe {
        *view = View {
            kind: n.kind as u32,
            signed: n.signed,
            unsigned: n.unsigned,
            number: n.number,
            text: n.text.as_ptr(),
            text_len: n.text.len(),
            type_name: n.type_name.as_ptr(),
            type_len: n.type_name.len(),
            name: n.name.as_ptr(),
            name_len: n.name.len(),
            child_count: n.children.len(),
        };
    }
}
